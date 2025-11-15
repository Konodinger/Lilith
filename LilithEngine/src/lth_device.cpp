#include "lth_device.hpp"
#include "lth_compile_options.hpp"

// std headers
#include <cstring>
#include <iostream>
#include <set>
#include <unordered_set>
#include <Windows.h>

namespace lth {

    // local callback functions
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData) {

        auto console_color = GetStdHandle(STD_OUTPUT_HANDLE);
        std::string severityFlag;
        std::string typeFlag;
        
        switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: SetConsoleTextAttribute(console_color, 10); severityFlag = "verbose severity - "; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: SetConsoleTextAttribute(console_color, 9); severityFlag = "info severity - "; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: SetConsoleTextAttribute(console_color, 14); severityFlag = "warning severity - "; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: SetConsoleTextAttribute(console_color, 12); severityFlag = "error severity - "; break;
        }
        switch (messageType) {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: typeFlag = "general type: "; break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: typeFlag = "validation type: "; break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: typeFlag = "performance type: "; break;
        }
        std::cerr << "Validation layer - " << severityFlag << typeFlag << std::endl;
        SetConsoleTextAttribute(console_color, 7);
        std::cerr << pCallbackData->pMessage << std::endl << std::endl;

        return VK_FALSE;
    }

    static void checkVkResult(VkResult err)
    {
        if (err == 0)
            return;
        std::cerr << "[ImGui-Vulkan] Error: VkResult = " << err << std::endl;
        if (err < 0)
            abort();
    }

    VkResult CreateDebugUtilsMessengerEXT(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
        const VkAllocationCallbacks *pAllocator,
        VkDebugUtilsMessengerEXT *pDebugMessenger) {
      auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          instance,
          "vkCreateDebugUtilsMessengerEXT");
      if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
      } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
      }
    }

    void DestroyDebugUtilsMessengerEXT(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks *pAllocator) {
      auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          instance,
          "vkDestroyDebugUtilsMessengerEXT");
      if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
      }
    }

    // class member functions
    LthDevice::LthDevice(LthWindow &window) : window{window} {
        rayTracingProperties.pNext = &accelStructProperties;
        physicalDeviceProperties.pNext = &rayTracingProperties;

        accelStructFeatures.accelerationStructure = VK_TRUE;
        rayTracingFeatures.pNext = &accelStructFeatures;
        rayTracingFeatures.rayTracingPipeline = VK_TRUE;
        physicalDeviceFeatures1_2.pNext = &rayTracingFeatures;
        physicalDeviceFeatures1_2.bufferDeviceAddress = VK_TRUE;
        physicalDeviceFeatures2.pNext = &physicalDeviceFeatures1_2;
        physicalDeviceFeatures2.features.samplerAnisotropy = VK_TRUE;
        physicalDeviceFeatures2.features.sampleRateShading = VK_TRUE;

        rayTracingFeatures_Get.pNext = &accelStructFeatures_Get;
        physicalDeviceFeatures1_2_Get.pNext = &rayTracingFeatures_Get;
        physicalDeviceFeatures2_Get.pNext = &physicalDeviceFeatures1_2_Get;

        if (volkInitialize() != VK_SUCCESS) {
            throw std::runtime_error("Failed to initialize Volk!");
        }

      createInstance();
      setupDebugMessenger();
      createSurface();
      pickPhysicalDevice();
      createLogicalDevice();
      createCommandPool();
    }

    LthDevice::~LthDevice() {
      vkDestroyCommandPool(device, commandPool, nullptr);
      vkDestroyDevice(device, nullptr);

      if (LTH_ENABLE_VALIDATION_LAYERS) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
      }

      vkDestroySurfaceKHR(instance, surface, nullptr);
      vkDestroyInstance(instance, nullptr);
    }

    void LthDevice::createInstance() {
      if (LTH_ENABLE_VALIDATION_LAYERS && !checkValidationLayerSupport()) {
        throw std::runtime_error("Validation layers requested, but not available!");
      }

      VkApplicationInfo appInfo = {};
      appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
      appInfo.pApplicationName = "Lilith Engine";
      appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
      appInfo.pEngineName = "No Engine";
      appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
      appInfo.apiVersion = VK_API_VERSION_1_4;

      VkInstanceCreateInfo createInfo = {};
      createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
      createInfo.pApplicationInfo = &appInfo;

      auto extensions = getRequiredExtensions();
      createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
      createInfo.ppEnabledExtensionNames = extensions.data();

      VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
      VkValidationFeaturesEXT features = {};
      if (LTH_ENABLE_VALIDATION_LAYERS) {
        features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
        features.enabledValidationFeatureCount = static_cast<uint32_t>(LTH_VALIDATION_FEATURE_ENABLES_LIST.size());
        features.pEnabledValidationFeatures = LTH_VALIDATION_FEATURE_ENABLES_LIST.data();

        createInfo.enabledLayerCount = static_cast<uint32_t>(LTH_VALIDATION_LAYERS_LIST.size());
        createInfo.ppEnabledLayerNames = LTH_VALIDATION_LAYERS_LIST.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);

        debugCreateInfo.pNext = &features;

        createInfo.pNext = &debugCreateInfo;
      } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
      }

      if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create instance!");
      }

      volkLoadInstanceOnly(instance);

      hasGflwRequiredInstanceExtensions();
    }

    void LthDevice::pickPhysicalDevice() {
      uint32_t deviceCount = 0;
      vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
      if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
      }
      //std::cout << "Device count: " << deviceCount << std::endl;
      std::vector<VkPhysicalDevice> devices(deviceCount);
      vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

      for (const auto &device : devices) {
        if (isDeviceSuitable(device)) {
          physicalDevice = device;
          break;
        }
      }

      if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable GPU!");
      }

      vkGetPhysicalDeviceProperties2(physicalDevice, &physicalDeviceProperties);
     
      // Section to print GPU properties. 

      std::cout << "Physical device: " << physicalDeviceProperties.properties.deviceName << std::endl;
      
      //msaaSamples = getMaxUsableSampleCount();
    }

    void LthDevice::createLogicalDevice() {
      QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

      std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
      std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsAndComputeFamily, indices.presentFamily};

      float queuePriority = 1.0f;
      for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
      }

      VkDeviceCreateInfo createInfo{};
      createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

      createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
      createInfo.pQueueCreateInfos = queueCreateInfos.data();

      createInfo.pNext = &physicalDeviceFeatures2;
      createInfo.pEnabledFeatures = nullptr; //legacy, not adapted to extension features.
      createInfo.enabledExtensionCount = static_cast<uint32_t>(LTH_DEVICE_EXTENSIONS_LIST.size());
      createInfo.ppEnabledExtensionNames = LTH_DEVICE_EXTENSIONS_LIST.data();

      // Might not really be necessary anymore because device specific validation layers
      // have been deprecated.
      if (LTH_ENABLE_VALIDATION_LAYERS) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(LTH_VALIDATION_LAYERS_LIST.size());
        createInfo.ppEnabledLayerNames = LTH_VALIDATION_LAYERS_LIST.data();
      } else {
        createInfo.enabledLayerCount = 0;
      }

      if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device!");
      }

      volkLoadDevice(device);

      vkGetDeviceQueue(device, indices.graphicsAndComputeFamily, 0, &graphicsQueue);
      vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
      vkGetDeviceQueue(device, indices.graphicsAndComputeFamily, 0, &computeQueue);
    }

    void LthDevice::createCommandPool() {
      QueueFamilyIndices queueFamilyIndices = findPhysicalQueueFamilies();

      VkCommandPoolCreateInfo poolInfo = {};
      poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsAndComputeFamily;
      poolInfo.flags =
          VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

      if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool!");
      }
    }

    void LthDevice::createSurface() { window.createWindowSurface(instance, &surface); }

    bool LthDevice::isDeviceSuitable(VkPhysicalDevice device) {
      QueueFamilyIndices indices = findQueueFamilies(device);

      bool extensionsSupported = checkDeviceExtensionSupport(device);

      bool swapChainAdequate = false;
      if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
      }

      vkGetPhysicalDeviceFeatures2(device, &physicalDeviceFeatures2_Get);

      return indices.isComplete() && extensionsSupported && swapChainAdequate &&
          physicalDeviceFeatures2_Get.features.samplerAnisotropy;
    }

    void LthDevice::populateDebugMessengerCreateInfo(
        VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
      createInfo = {};
      createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
      createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
      createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
      createInfo.pfnUserCallback = debugCallback;
      createInfo.pUserData = nullptr;  // Optional
    }

    void LthDevice::setupDebugMessenger() {
      if (!LTH_ENABLE_VALIDATION_LAYERS) return;
      VkDebugUtilsMessengerCreateInfoEXT createInfo;
      populateDebugMessengerCreateInfo(createInfo);
      if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("Failed to set up debug messenger!");
      }
    }

    bool LthDevice::checkValidationLayerSupport() {
      uint32_t layerCount;
      vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

      std::vector<VkLayerProperties> availableLayers(layerCount);
      vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

      for (const char *layerName : LTH_VALIDATION_LAYERS_LIST) {
        bool layerFound = false;

        for (const auto &layerProperties : availableLayers) {
          if (strcmp(layerName, layerProperties.layerName) == 0) {
            layerFound = true;
            break;
          }
        }

        if (!layerFound) {
          return false;
        }
      }

      return true;
    }

    std::vector<const char *> LthDevice::getRequiredExtensions() {
      uint32_t glfwExtensionCount = 0;
      const char **glfwExtensions;
      glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

      std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

      if (LTH_ENABLE_VALIDATION_LAYERS) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
      }

      return extensions;
    }

    void LthDevice::hasGflwRequiredInstanceExtensions() {
      uint32_t extensionCount = 0;
      vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
      std::vector<VkExtensionProperties> extensions(extensionCount);
      vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

      //std::cout << "available extensions:" << std::endl;
      std::unordered_set<std::string> available;
      for (const auto &extension : extensions) {
        //std::cout << "\t" << extension.extensionName << std::endl;
        available.insert(extension.extensionName);
      }

      //std::cout << "required extensions:" << std::endl;
      auto requiredExtensions = getRequiredExtensions();
      for (const auto &required : requiredExtensions) {
        //std::cout << "\t" << required << std::endl;
        if (available.find(required) == available.end()) {
          throw std::runtime_error("Missing required glfw extension.");
        }
      }
    }

    bool LthDevice::checkDeviceExtensionSupport(VkPhysicalDevice device) {
      uint32_t extensionCount;
      vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

      std::vector<VkExtensionProperties> availableExtensions(extensionCount);
      vkEnumerateDeviceExtensionProperties(
          device,
          nullptr,
          &extensionCount,
          availableExtensions.data());

      std::set<std::string> requiredExtensions(LTH_DEVICE_EXTENSIONS_LIST.begin(), LTH_DEVICE_EXTENSIONS_LIST.end());

      for (const auto &extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
      }

      return requiredExtensions.empty();
    }

    QueueFamilyIndices LthDevice::findQueueFamilies(VkPhysicalDevice device) {
      QueueFamilyIndices indices;

      uint32_t queueFamilyCount = 0;
      vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

      std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
      vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

      int i = 0;
      for (const auto &queueFamily : queueFamilies) {
        if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
          indices.graphicsAndComputeFamily = i;
          indices.graphicsAndComputeFamilyHasValue = true;
        }
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (queueFamily.queueCount > 0 && presentSupport) {
          indices.presentFamily = i;
          indices.presentFamilyHasValue = true;
        }
        if (indices.isComplete()) {
          break;
        }

        i++;
      }

      return indices;
    }

    SwapChainSupportDetails LthDevice::querySwapChainSupport(VkPhysicalDevice device) {
      SwapChainSupportDetails details;
      vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

      uint32_t formatCount;
      vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

      if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
      }

      uint32_t presentModeCount;
      vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

      if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            device,
            surface,
            &presentModeCount,
            details.presentModes.data());
      }
      return details;
    }

    VkFormat LthDevice::findSupportedFormat(
        const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
      for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
          return format;
        } else if (
            tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
          return format;
        }
      }
      throw std::runtime_error("Failed to find supported format!");
    }

    ImGui_ImplVulkan_InitInfo LthDevice::getImGuiInitInfo(VkDescriptorPool descriptorPool, uint32_t imageCount) {
        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.Instance = instance;
        initInfo.PhysicalDevice = physicalDevice;
        initInfo.Device = device;
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);
        assert(queueFamilyIndices.graphicsAndComputeFamilyHasValue && "Error: could not init ImGui, graphics queue has no family!");
        initInfo.QueueFamily = queueFamilyIndices.graphicsAndComputeFamily;
        initInfo.Queue = graphicsQueue;
        initInfo.PipelineCache = VK_NULL_HANDLE;
        initInfo.DescriptorPool = descriptorPool;
        initInfo.Allocator = nullptr;
        initInfo.MinImageCount = imageCount;
        initInfo.ImageCount = imageCount;
        initInfo.CheckVkResultFn = checkVkResult;
        return initInfo;
    }

    uint32_t LthDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
      VkPhysicalDeviceMemoryProperties memProperties;
      vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
      for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
          return i;
        }
      }

      throw std::runtime_error("Failed to find suitable memory type!");
    }

    void LthDevice::createBuffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer &buffer,
        VkDeviceMemory &bufferMemory) {
      VkBufferCreateInfo bufferInfo{};
      bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      bufferInfo.size = size;
      bufferInfo.usage = usage;
      bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

      if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer!");
      }

      VkMemoryRequirements memRequirements;
      vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

      VkMemoryAllocateFlagsInfo allocFlagsInfo{};
      allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
      allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

      VkMemoryAllocateInfo allocInfo{};
      allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      allocInfo.pNext = &allocFlagsInfo;
      allocInfo.allocationSize = memRequirements.size;
      allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

      if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate buffer memory!");
      }

      vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }

    VkCommandBuffer LthDevice::beginSingleTimeCommands() {
      VkCommandBufferAllocateInfo allocInfo{};
      allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      allocInfo.commandPool = commandPool;
      allocInfo.commandBufferCount = 1;

      VkCommandBuffer commandBuffer;
      if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers!");
    }

      VkCommandBufferBeginInfo beginInfo{};
      beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

      if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
          throw std::runtime_error("Failed to begin recording command buffer!");
      }
      return commandBuffer;
    }

    void LthDevice::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
      vkEndCommandBuffer(commandBuffer);

      VkSubmitInfo submitInfo{};
      submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submitInfo.commandBufferCount = 1;
      submitInfo.pCommandBuffers = &commandBuffer;

      vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
      vkQueueWaitIdle(graphicsQueue);

      vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    void LthDevice::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
      VkCommandBuffer commandBuffer = beginSingleTimeCommands();

      VkBufferCopy copyRegion{};
      copyRegion.srcOffset = 0;  // Optional
      copyRegion.dstOffset = 0;  // Optional
      copyRegion.size = size;
      vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

      endSingleTimeCommands(commandBuffer);
    }

    void LthDevice::copyBufferToImage(
        VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount) {
      VkCommandBuffer commandBuffer = beginSingleTimeCommands();

      VkBufferImageCopy region{};
      region.bufferOffset = 0;
      region.bufferRowLength = 0;
      region.bufferImageHeight = 0;

      region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      region.imageSubresource.mipLevel = 0;
      region.imageSubresource.baseArrayLayer = 0;
      region.imageSubresource.layerCount = layerCount;

      region.imageOffset = {0, 0, 0};
      region.imageExtent = {width, height, 1};

      vkCmdCopyBufferToImage(
          commandBuffer,
          buffer,
          image,
          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
          1,
          &region);
      endSingleTimeCommands(commandBuffer);
    }


    void LthDevice::buildAccelerationStructure(const VkAccelerationStructureBuildGeometryInfoKHR &asBuildGeometryInfo, const VkAccelerationStructureBuildRangeInfoKHR& asBuildRangeInfo) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();
        auto asBuildRangeInfos = { &asBuildRangeInfo };
        vkCmdBuildAccelerationStructuresKHR(
            commandBuffer,
            1,
            &asBuildGeometryInfo,
            asBuildRangeInfos.begin());

        endSingleTimeCommands(commandBuffer);
    }

    void LthDevice::createImage(uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkImage& image,
        VkDeviceMemory& imageMemory,
        uint32_t mipLevels,
        VkSampleCountFlagBits numSamples) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = numSamples;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags = 0;

        createImageWithInfo(imageInfo, properties, image, imageMemory);
    }

    void LthDevice::createImageWithInfo(
        const VkImageCreateInfo& imageInfo,
        VkMemoryPropertyFlags properties,
        VkImage& image,
        VkDeviceMemory& imageMemory) {
      if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image!");
      }

      VkMemoryRequirements memRequirements;
      vkGetImageMemoryRequirements(device, image, &memRequirements);

      VkMemoryAllocateInfo allocInfo{};
      allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      allocInfo.allocationSize = memRequirements.size;
      allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

      if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate image memory!");
      }

      if (vkBindImageMemory(device, image, imageMemory, 0) != VK_SUCCESS) {
        throw std::runtime_error("Failed to bind image memory!");
      }
    }

    void LthDevice::transitionImageLayout(
        VkImage image,
        VkFormat format,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        uint32_t mipLevels) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

            if (hasStencilComponent(format)) {
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        }
        else {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        else {
            throw std::invalid_argument("Unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        endSingleTimeCommands(commandBuffer);
    }

    VkImageView LthDevice::createImageView(
        VkImage image,
        VkFormat format,
        VkImageAspectFlags aspectFlags,
        uint32_t mipLevels) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;

        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create texture image view!");
        }

        return imageView;
    }

    void LthDevice::generateMipmaps(
        VkImage image,
        VkFormat format,
        int32_t texWidth,
        int32_t texHeight,
        uint32_t mipLevels) {

        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);

        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            throw std::runtime_error("Texture image format does not support linear blitting!");
        }

        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        for (uint32_t i = 1; i < mipLevels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            VkImageBlit blit{};
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = { 0, 0, 0 };

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;

            blit.dstOffsets[1] = { mipWidth, mipHeight, 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(commandBuffer,
                image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit,
                VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        endSingleTimeCommands(commandBuffer);
    }

    VkSampleCountFlagBits LthDevice::getMaxUsableSampleCount() {
        VkSampleCountFlags counts = physicalDeviceProperties.properties.limits.framebufferColorSampleCounts
            & physicalDeviceProperties.properties.limits.framebufferDepthSampleCounts; //If a stencil is implemented, the device also needs to get framebufferStencilSampleCounts.
        for (auto flagBit : { VK_SAMPLE_COUNT_64_BIT, VK_SAMPLE_COUNT_32_BIT, VK_SAMPLE_COUNT_16_BIT, VK_SAMPLE_COUNT_8_BIT, VK_SAMPLE_COUNT_4_BIT, VK_SAMPLE_COUNT_2_BIT }) {
            if (counts & flagBit) return flagBit;
        }
        return VK_SAMPLE_COUNT_1_BIT;
    }

    bool LthDevice::hasStencilComponent(VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

}