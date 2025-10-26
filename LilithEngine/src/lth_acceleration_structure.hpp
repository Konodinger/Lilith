#ifndef __LTH_ACCELERATION_STRUCTURE_HPP__
#define __LTH_ACCELERATION_STRUCTURE_HPP__

#include "lth_buffer.hpp"

#include <memory>

namespace lth {


    struct AccelerationStructure
    {
        VkAccelerationStructureKHR handle;
        VkDeviceAddress            deviceAddress;
        std::unique_ptr<LthBuffer> buffer;
    };
}

#endif