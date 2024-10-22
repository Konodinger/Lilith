#ifndef __LTH_RENDERER_HPP__
#define __LTH_RENDERER_HPP__

#include "lth_window.hpp"
#include "lth_device.hpp"
#include "lth_swap_chain.hpp"

#include <memory>
#include <vector>
#include <cassert>

namespace lth {

	class LthRenderer {
	public:

		LthRenderer(LthWindow& window, LthDevice& device);
		~LthRenderer();

		LthRenderer(const LthRenderer&) = delete;
		LthRenderer& operator=(const LthRenderer&) = delete;

		VkRenderPass getSwapChainRenderPass() const { return lthSwapChain->getRenderPass(); }
		size_t getSwapChainImageCount() const { return lthSwapChain->imageCount(); }
		VkFormat getSwapChainImageFormat() const { return lthSwapChain->getSwapChainImageFormat(); }
		float getAspectRatio() const { return lthSwapChain->extentAspectRatio(); }
		bool isFrameInProgress() const { return isFrameStarted; }

		VkCommandBuffer getCurrentGraphicsCommandBuffer() const {
			assert(isFrameStarted && "Cannot get graphics command buffer when frame not in progress.");
			return graphicsCommandBuffers[currentFrameIndex];
		}

		VkCommandBuffer getCurrentComputeCommandBuffer() const {
			assert(isFrameStarted && "Cannot get compute command buffer when frame not in progress.");
			return computeCommandBuffers[currentFrameIndex];
		}

		int getFrameIndex() const {
			assert(isFrameStarted && "Cannot get frame index when frame not in progress.");
			return currentFrameIndex;
		}

		bool beginFrame();
		bool beginComputes();
		void endFrame();
		void endComputes();
		void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
		void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

	private:
		void createCommandBuffers(std::vector<VkCommandBuffer>& commandBuffers);
		void freeCommandBuffers(std::vector<VkCommandBuffer>& commandBuffers);
		void recreateSwapChain();

		LthWindow& lthWindow;
		LthDevice& lthDevice;
		std::unique_ptr<LthSwapChain>  lthSwapChain;
		std::vector<VkCommandBuffer> graphicsCommandBuffers;
		std::vector<VkCommandBuffer> computeCommandBuffers;

		uint32_t currentImageIndex;
		int currentFrameIndex{ 0 };
		bool isFrameStarted{ false };
	};
}

#endif