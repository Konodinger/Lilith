#ifndef __LTH_COMPILE_OPTIONS_HPP__
#define __LTH_COMPILE_OPTIONS_HPP__

#include <vulkan/vulkan.h>
#include <vector>

//
// This is the configuration section, where you can enable or disable options at compile time. This will mostly be used for debugging.
// For now, there are two options: enabling validation layers, and enabling best practice validation layers. (Logically, the first needs to be enabled for the second to have any effect)
//

#define LTH_VK_ENABLE_VL

#ifdef LTH_VK_ENABLE_VL
//#define LTH_VK_ENABLE_BEST_PRACTICES_VL
#endif

//
// End of configuration section
//

namespace lth {

    // Validation layers and vulkan extensions section
#ifdef LTH_VK_ENABLE_VL
    const bool LTH_ENABLE_VALIDATION_LAYERS = true;
    const std::vector<const char*> LTH_VALIDATION_LAYERS_LIST = { "VK_LAYER_KHRONOS_validation" };
#else
    const bool LTH_ENABLE_VALIDATION_LAYERS = false;
    const std::vector<const char*> LTH_VALIDATION_LAYERS_LIST{};
#endif

#ifdef LTH_VK_ENABLE_BEST_PRACTICES_VL
    const std::vector<VkValidationFeatureEnableEXT> LTH_VALIDATION_FEATURE_ENABLES_LIST = { VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT };
#else
    const std::vector<VkValidationFeatureEnableEXT> LTH_VALIDATION_FEATURE_ENABLES_LIST{};
#endif

    const std::vector<const char*> LTH_DEVICE_EXTENSIONS_LIST =
        { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME };
}

#endif