#include "lth_renderer.hpp"

#include <cassert>
#include <stdexcept>
#include <array>

namespace lth {

	LthRenderer::LthRenderer(LthWindow& window, LthDevice& device) : lthWindow{ window }, lthDevice{ device } {
		recreateSwapChain();
		createCommandBuffers(graphicsCommandBuffers);
		createCommandBuffers(computeCommandBuffers);
	}

	LthRenderer::~LthRenderer() {
		freeCommandBuffers(graphicsCommandBuffers);
		freeCommandBuffers(computeCommandBuffers);
	}

	void LthRenderer::recreateSwapChain() {
		auto extent = lthWindow.getExtent();
		while (extent.width == 0 || extent.height == 0) {
			extent = lthWindow.getExtent();
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(lthDevice.getDevice());

		if (lthSwapChain == nullptr) {
			lthSwapChain = std::make_unique<LthSwapChain>(lthDevice, extent);
		} else {
			std::shared_ptr<LthSwapChain> oldSwapChain = std::move(lthSwapChain);
			lthSwapChain = std::make_unique<LthSwapChain>(lthDevice, extent, oldSwapChain);
			
			if (!oldSwapChain->compareSwapFormats(*lthSwapChain.get())) {
				throw std::runtime_error("Swap chain image (or depth) format has changed!");
			}

			//ImGui_ImplVulkan_SetMinImageCount(static_cast<uint32_t>(lthSwapChain->imageCount()));
			//ImGui_ImplVulkanH_CreateWindow ?
		}

		//To complete.
	}

	void LthRenderer::createCommandBuffers(std::vector<VkCommandBuffer>& commandBuffers) {
		commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = lthDevice.getCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		if (vkAllocateCommandBuffers(lthDevice.getDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate command buffers!");
		}
	};

	void LthRenderer::freeCommandBuffers(std::vector<VkCommandBuffer>& commandBuffers) {
		vkFreeCommandBuffers(
			lthDevice.getDevice(),
			lthDevice.getCommandPool(),
			static_cast<uint32_t>(commandBuffers.size()),
			commandBuffers.data());
		commandBuffers.clear();
	}

	void LthRenderer::waitForSwapChainWork() {
		lthSwapChain->waitForFrameFences(true);
	}

	bool LthRenderer::beginFrame() {
		assert(!isFrameStarted && "Can't call beginFrame while already in progress.");
		
		auto result = lthSwapChain->acquireNextImage(&currentImageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return false;
		}
		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) { //VK_SUBOPTIMAL_KHR will need to be handled, for instance for window resizing.
			throw std::runtime_error("Failed to acquire swap chain image!");
		}

		isFrameStarted = true;

		auto graphicsCommandBuffer = getCurrentGraphicsCommandBuffer();

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		if (vkBeginCommandBuffer(graphicsCommandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to begin recording graphics command buffer" + std::to_string(currentImageIndex) + "!");
		}


		return true;
	}

	bool LthRenderer::beginComputes() {

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		auto computeCommandBuffer = getCurrentComputeCommandBuffer();

		if (vkBeginCommandBuffer(computeCommandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to begin recording graphics command buffer" + std::to_string(currentImageIndex) + "!");
		}

		return true;
	}

	void LthRenderer::endFrame() {
		assert(isFrameStarted && "Can't call endFrame while frame is not in progress.");

		auto graphicsCommandBuffer = getCurrentGraphicsCommandBuffer();

		if (vkEndCommandBuffer(graphicsCommandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to record graphics command buffer " + std::to_string(currentImageIndex) + "!");
		}
		lthSwapChain->submitGraphicsCommandBuffers(&graphicsCommandBuffer, currentImageIndex);
		
		auto result = lthSwapChain->presentAndEndFrame(currentImageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || lthWindow.wasWindowResized()) {
			lthWindow.resetWindowResizedFlag();
			recreateSwapChain();
		} else if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to present swap chain image!");
		}

		isFrameStarted = false;
		currentFrameIndex = (currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void LthRenderer::endComputes() {

		auto computeCommandBuffer = getCurrentComputeCommandBuffer();

		if (vkEndCommandBuffer(computeCommandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to record compute command buffer " + std::to_string(currentImageIndex) + "!");
		}

		lthSwapChain->submitComputeCommandBuffers(&computeCommandBuffer, currentImageIndex);

	}

	void LthRenderer::beginSwapChainRenderPass(VkCommandBuffer graphicsCommandBuffer, RenderPassType renderPassType) {
		assert(isFrameStarted && "Can't call beginSwapChainRenderPass if frame is not in progress.");
		assert(graphicsCommandBuffer == getCurrentGraphicsCommandBuffer() && "Can't begin render pass on command buffer from a different frame.");

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

		switch (renderPassType) {
		case LTH_RP_MAIN:
			renderPassInfo.renderPass = lthSwapChain->getMainRenderPass();
			renderPassInfo.framebuffer = lthSwapChain->getMainFrameBuffer(currentImageIndex);
			break;
		case LTH_RP_GUI:
			renderPassInfo.renderPass = lthSwapChain->getGuiRenderPass();
			renderPassInfo.framebuffer = lthSwapChain->getGuiFrameBuffer(currentImageIndex);
			break;
		}

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = lthSwapChain->getSwapChainExtent();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.f };
		clearValues[1].depthStencil = { 1.f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(graphicsCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(lthSwapChain->getSwapChainExtent().width);
		viewport.height = static_cast<float>(lthSwapChain->getSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{ {0, 0}, lthSwapChain->getSwapChainExtent() };
		vkCmdSetViewport(graphicsCommandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(graphicsCommandBuffer, 0, 1, &scissor);

	}

	void LthRenderer::endSwapChainRenderPass(VkCommandBuffer graphicsCommandBuffer) {
		assert(isFrameStarted && "Can't call endSwapChainRenderPass if frame is not in progress.");
		assert(graphicsCommandBuffer == getCurrentGraphicsCommandBuffer() && "Can't end render pass on command buffer from a different frame.");

		vkCmdEndRenderPass(graphicsCommandBuffer);
	}

	void LthRenderer::copyImageToSwapChain(LthTexture& image) {
		auto graphicsCommandBuffer = getCurrentGraphicsCommandBuffer();
		lthSwapChain->copyImageToSwapChain(graphicsCommandBuffer, image, currentImageIndex);
	}

}