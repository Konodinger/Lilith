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

		VkCommandBuffer getCurrentCommandBuffer() const {
			assert(isFrameStarted && "Cannot get command buffer when frame not in progress.");
			return commandBuffers[currentFrameIndex];
		}

		int getFrameIndex() const {
			assert(isFrameStarted && "Cannot get frame index when frame not in progress.");
			return currentFrameIndex;
		}

		VkCommandBuffer beginFrame();
		void endFrame();
		void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
		void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

	private:
		void createCommandBuffers();
		void freeCommandBuffers();
		void recreateSwapChain();

		LthWindow& lthWindow;
		LthDevice& lthDevice;
		std::unique_ptr<LthSwapChain>  lthSwapChain;
		std::vector<VkCommandBuffer> commandBuffers;

		uint32_t currentImageIndex;
		int currentFrameIndex{ 0 };
		bool isFrameStarted{ false };
	};
}

#endif