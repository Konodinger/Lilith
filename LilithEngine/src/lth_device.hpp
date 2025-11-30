#ifndef __LTH_DEVICE_HPP__
#define __LTH_DEVICE_HPP__

#include "lth_window.hpp"

#include "backends/imgui_impl_vulkan.h"

#include <string>
#include <vector>

namespace lth {

    struct SwapChainSupportDetails {
      VkSurfaceCapabilitiesKHR capabilities;
      std::vector<VkSurfaceFormatKHR> formats;
      std::vector<VkPresentModeKHR> presentModes;
    };

    struct QueueFamilyIndices {
      uint32_t graphicsAndComputeFamily;
      uint32_t presentFamily;
      bool graphicsAndComputeFamilyHasValue = false;
      bool presentFamilyHasValue = false;
      bool isComplete() { return graphicsAndComputeFamilyHasValue && presentFamilyHasValue; }
    };

    class LthDevice {
     public:

      LthDevice(LthWindow &window);
      ~LthDevice();

      LthDevice(const LthDevice &) = delete;
      LthDevice &operator=(const LthDevice &) = delete;
      LthDevice(LthDevice &&) = default;
      LthDevice &operator=(LthDevice &&) = default;

      VkInstance const& getInstance() { return instance; }
      VkCommandPool getCommandPool() { return commandPool; }
      VkDevice getDevice() { return device; }
      LthWindow const& getWindow() { return window; }
      VkSurfaceKHR getSurface() { return surface; }
      VkQueue getGraphicsQueue() { return graphicsQueue; }
      VkQueue getPresentQueue() { return presentQueue; }
      VkQueue getComputeQueue() { return computeQueue; }

      // ImGui methods
      ImGui_ImplVulkan_InitInfo getImGuiInitInfo(VkDescriptorPool descriptorPool, uint32_t imageCount);

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
      void buildAccelerationStructure(const VkAccelerationStructureBuildGeometryInfoKHR& asBuildGeometryInfo, const VkAccelerationStructureBuildRangeInfoKHR& asBuildRangeInfo);

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
          uint32_t mipLevels,
          VkImageLayout newImageLayout);

      // Properties helper methods
      VkSampleCountFlagBits getMsaaSamples() { return msaaSamples; }
      bool isMsaaEnabled() { return (msaaSamples != VK_SAMPLE_COUNT_1_BIT); }


      VkPhysicalDeviceProperties2 physicalDeviceProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
      // Ray tracing extensions
      VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProperties{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
      VkPhysicalDeviceAccelerationStructurePropertiesKHR accelStructProperties{
          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR };

      
      VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
      VkPhysicalDeviceVulkan12Features physicalDeviceFeatures1_2{
          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
      VkPhysicalDeviceVulkan13Features physicalDeviceFeatures1_3{
          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
      VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingFeatures{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
      VkPhysicalDeviceAccelerationStructureFeaturesKHR accelStructFeatures{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };

      VkPhysicalDeviceFeatures2 physicalDeviceFeatures2_Get{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
      VkPhysicalDeviceVulkan12Features physicalDeviceFeatures1_2_Get{
          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
      VkPhysicalDeviceVulkan13Features physicalDeviceFeatures1_3_Get{
          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
      VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingFeatures_Get{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
      VkPhysicalDeviceAccelerationStructureFeaturesKHR accelStructFeatures_Get{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };

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


      VkDevice device;
      VkSurfaceKHR surface;
      VkQueue graphicsQueue, presentQueue, computeQueue;

      VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    };

}

#endif