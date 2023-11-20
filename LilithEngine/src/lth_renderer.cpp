#include "lth_renderer.hpp"

#include <cassert>
#include <stdexcept>
#include <array>

namespace lth {

	LthRenderer::LthRenderer(LthWindow& window, LthDevice& device) : lthWindow{ window }, lthDevice{ device } {
		recreateSwapChain();
		createCommandBuffers();
	}

	LthRenderer::~LthRenderer() {
		freeCommandBuffers();
	}

	void LthRenderer::recreateSwapChain() {
		auto extent = lthWindow.getExtent();
		while (extent.width == 0 || extent.height == 0) {
			extent = lthWindow.getExtent();
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(lthDevice.device());

		if (lthSwapChain == nullptr) {
			lthSwapChain = std::make_unique<LthSwapChain>(lthDevice, extent);
		} else {
			std::shared_ptr<LthSwapChain> oldSwapChain = std::move(lthSwapChain);
			lthSwapChain = std::make_unique<LthSwapChain>(lthDevice, extent, oldSwapChain);
			
			if (!oldSwapChain->compareSwapFormats(*lthSwapChain.get())) {
				throw std::runtime_error("Swap chain image (or depth) format has changed!");
			}
		}

		//To complete.
	}

	void LthRenderer::createCommandBuffers() {
		commandBuffers.resize(LthSwapChain::MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = lthDevice.getCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		if (vkAllocateCommandBuffers(lthDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate command buffers!");
		}
	};

	void LthRenderer::freeCommandBuffers() {
		vkFreeCommandBuffers(
			lthDevice.device(),
			lthDevice.getCommandPool(),
			static_cast<uint32_t>(commandBuffers.size()),
			commandBuffers.data());
		commandBuffers.clear();
	}

	VkCommandBuffer LthRenderer::beginFrame() {
		assert(!isFrameStarted && "Can't call beginFrame while already in progress.");
		
		auto result = lthSwapChain->acquireNextImage(&currentImageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return nullptr;
		}
		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) { //VK_SUBOPTIMAL_KHR will need to be handled, for instance for window resizing.
			throw std::runtime_error("Failed to acquire swap chain image!");
		}

		isFrameStarted = true;

		auto commandBuffer = getCurrentCommandBuffer();

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to begin recording command buffer" + std::to_string(currentImageIndex) + "!");
		}

		return commandBuffer;
	}

	void LthRenderer::endFrame() {
		assert(isFrameStarted && "Can't call endFrame while frame is not in progress.");

		auto commandBuffer = getCurrentCommandBuffer();

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to record command buffer " + std::to_string(currentImageIndex) + "!");
		}

		auto result = lthSwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || lthWindow.wasWindowResized()) {
			lthWindow.resetWindowResizedFlag();
			recreateSwapChain();
		} else if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to present swap chain image!");
		}

		isFrameStarted = false;
		currentFrameIndex = (currentFrameIndex + 1) % LthSwapChain::MAX_FRAMES_IN_FLIGHT;
	}
	void LthRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) {
		assert(isFrameStarted && "Can't call beginSwapChainRenderPass if frame is not in progress.");
		assert(commandBuffer == getCurrentCommandBuffer() && "Can't begin render pass on command buffer from a different frame.");

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = lthSwapChain->getRenderPass();
		renderPassInfo.framebuffer = lthSwapChain->getFrameBuffer(currentImageIndex);

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = lthSwapChain->getSwapChainExtent();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.f };
		clearValues[1].depthStencil = { 1.f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(lthSwapChain->getSwapChainExtent().width);
		viewport.height = static_cast<float>(lthSwapChain->getSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{ {0, 0}, lthSwapChain->getSwapChainExtent() };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	}

	void LthRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) {
		assert(isFrameStarted && "Can't call endSwapChainRenderPass if frame is not in progress.");
		assert(commandBuffer == getCurrentCommandBuffer() && "Can't end render pass on command buffer from a different frame.");

		vkCmdEndRenderPass(commandBuffer);
	}

}