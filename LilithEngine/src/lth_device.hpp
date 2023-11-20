#ifndef __LTH_DEVICE_HPP__
#define __LTH_DEVICE_HPP__

#include "lth_window.hpp"

#include <string>
#include <vector>

namespace lth {

    struct SwapChainSupportDetails {
      VkSurfaceCapabilitiesKHR capabilities;
      std::vector<VkSurfaceFormatKHR> formats;
      std::vector<VkPresentModeKHR> presentModes;
    };

    struct QueueFamilyIndices {
      uint32_t graphicsFamily;
      uint32_t presentFamily;
      bool graphicsFamilyHasValue = false;
      bool presentFamilyHasValue = false;
      bool isComplete() { return graphicsFamilyHasValue && presentFamilyHasValue; }
    };

    class LthDevice {
     public:

      LthDevice(LthWindow &window);
      ~LthDevice();

      LthDevice(const LthDevice &) = delete;
      LthDevice &operator=(const LthDevice &) = delete;
      LthDevice(LthDevice &&) = default;
      LthDevice &operator=(LthDevice &&) = default;

      VkCommandPool getCommandPool() { return commandPool; }
      VkDevice device() { return _device; }
      VkSurfaceKHR surface() { return _surface; }
      VkQueue graphicsQueue() { return _graphicsQueue; }
      VkQueue presentQueue() { return _presentQueue; }

      // Swap chain creation methods
      SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(physicalDevice); }
      uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
      QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(physicalDevice); }
      VkFormat findSupportedFormat(
          const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

      // Buffer helper methods
      void createBuffer(
          VkDeviceSize size,
          VkBufferUsageFlags usage,
          VkMemoryPropertyFlags properties,
          VkBuffer &buffer,
          VkDeviceMemory &bufferMemory);
      VkCommandBuffer beginSingleTimeCommands();
      void endSingleTimeCommands(VkCommandBuffer commandBuffer);
      void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
      void copyBufferToImage(
          VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);
  
      // Image generation methods
      void createImage(uint32_t width,
          uint32_t height,
          VkFormat format,
          VkImageTiling tiling,
          VkImageUsageFlags usage,
          VkMemoryPropertyFlags properties,
          VkImage& image,
          VkDeviceMemory& imageMemory,
          uint32_t mipLevels = 1,
          VkSampleCountFlagBits numSamples = VK_SAMPLE_COUNT_1_BIT);
      void createImageWithInfo(
          const VkImageCreateInfo &imageInfo,
          VkMemoryPropertyFlags properties,
          VkImage &image,
          VkDeviceMemory &imageMemory);
      void transitionImageLayout(
          VkImage image,
          VkFormat format,
          VkImageLayout oldLayout,
          VkImageLayout newLayout,
          uint32_t mipLevels = 1);
      VkImageView createImageView(
          VkImage image,
          VkFormat format,
          VkImageAspectFlags aspectFlags,
          uint32_t mipLevels = 1);
      void generateMipmaps(
          VkImage image,
          VkFormat imageFormat,
          int32_t texWidth,
          int32_t texHeight,
          uint32_t mipLevels);

      // Properties helper methods
      VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
      VkPhysicalDeviceProperties physicalDeviceProperties;

     private:
      void createInstance();
      void setupDebugMessenger();
      void createSurface();
      void pickPhysicalDevice();
      void createLogicalDevice();
      void createCommandPool();

      // helper methods
      bool isDeviceSuitable(VkPhysicalDevice device);
      std::vector<const char *> getRequiredExtensions();
      bool checkValidationLayerSupport();
      QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
      void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
      void hasGflwRequiredInstanceExtensions();
      bool checkDeviceExtensionSupport(VkPhysicalDevice device);
      SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
      VkSampleCountFlagBits getMaxUsableSampleCount();
      bool hasStencilComponent(VkFormat format);

      VkInstance instance;
      VkDebugUtilsMessengerEXT debugMessenger;
      VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
      LthWindow &window;
      VkCommandPool commandPool;


      VkDevice _device;
      VkSurfaceKHR _surface;
      VkQueue _graphicsQueue;
      VkQueue _presentQueue;
    };

}

#endif