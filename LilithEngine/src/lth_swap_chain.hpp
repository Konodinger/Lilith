#ifndef __LTH_SWAP_CHAIN_HPP__
#define __LTH_SWAP_CHAIN_HPP__

#include "lth_global_info.hpp"
#include "lth_device.hpp"

#include <string>
#include <vector>
#include <memory>

namespace lth {
    class LthTexture;

    enum RenderPassType {
        LTH_RP_MAIN,
        LTH_RP_GUI,
    };

class LthSwapChain {
 public:

  LthSwapChain(LthDevice &deviceRef, VkExtent2D windowExtent);
  LthSwapChain(LthDevice &deviceRef, VkExtent2D windowExtent, std::shared_ptr<LthSwapChain> previous);
  ~LthSwapChain();

  LthSwapChain(const LthSwapChain &) = delete;
  LthSwapChain &operator=(const LthSwapChain &) = delete;

  VkFramebuffer getMainFrameBuffer(int index) { return mainFramebuffers[index]; }
  VkFramebuffer getGuiFrameBuffer(int index) { return guiFramebuffers[index]; }
  VkRenderPass getMainRenderPass() { return mainRenderPass; }
  VkRenderPass getGuiRenderPass() { return guiRenderPass; }
  VkImageView getImageView(int index) { return swapChainImageViews[index]; }
  size_t imageCount() { return swapChainImages.size(); }
  VkFormat getSwapChainImageFormat() { return swapChainImageFormat; }
  VkExtent2D getSwapChainExtent() { return swapChainExtent; }
  uint32_t width() { return swapChainExtent.width; }
  uint32_t height() { return swapChainExtent.height; }

  float extentAspectRatio() {
    return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
  }
  VkFormat findDepthFormat();

  VkResult acquireNextImage(uint32_t *imageIndex);
  void copyImageToSwapChain(VkCommandBuffer commandBuffer, LthTexture& sourceImage, uint32_t imageIndex);
  void submitComputeCommandBuffers(const VkCommandBuffer * commandBuffer, uint32_t imageIndex);
  void submitGraphicsCommandBuffers(const VkCommandBuffer * commandBuffer, uint32_t imageIndex);
  VkResult presentAndEndFrame(uint32_t imageIndex);

  bool compareSwapFormats(const LthSwapChain& swapChain) const {
      return swapChain.swapChainDepthFormat == swapChainDepthFormat &&
          swapChain.swapChainImageFormat == swapChainImageFormat;
  }

 private:
  void init();
  void createSwapChain();
  void createImageViews();
  void createColorResources();
  void createDepthResources();
  void createRenderPass();
  void createFramebuffers();
  void createSyncObjects();

  // Helper functions
  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR> &availableFormats);
  VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes);
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

  VkFormat swapChainImageFormat;
  VkFormat swapChainDepthFormat;
  VkExtent2D swapChainExtent;

  std::vector<VkFramebuffer> mainFramebuffers;
  std::vector<VkFramebuffer> guiFramebuffers;
  VkRenderPass mainRenderPass;
  VkRenderPass guiRenderPass;

  std::vector<VkImage> colorImages;
  std::vector<VkDeviceMemory> colorImageMemories;
  std::vector<VkImageView> colorImageViews;
  std::vector<VkImage> depthImages;
  std::vector<VkDeviceMemory> depthImageMemories;
  std::vector<VkImageView> depthImageViews;

  std::vector<VkImage> swapChainImages;
  std::vector<VkImageView> swapChainImageViews;

  LthDevice &lthDevice;
  VkExtent2D windowExtent;

  VkSwapchainKHR swapChain;
  std::shared_ptr<LthSwapChain> oldSwapChain;

  std::vector<std::pair<VkSemaphore, bool>> computeFinishedSemaphores; // The bool is to check if the semaphore is signaled.
  std::vector<std::pair<VkSemaphore, bool>> renderFinishedSemaphores;
  std::vector<std::pair<VkSemaphore, bool>> imageAvailableSemaphores;
  std::vector<VkFence> computeInFlightFences;
  std::vector<VkFence> renderInFlightFences;
  size_t currentFrame = 0;
};

}

#endif