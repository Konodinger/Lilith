#include "lth_acceleration_structure.hpp"

namespace lth {

	void buildAccelerationStructure(LthDevice& device,
		VkAccelerationStructureTypeKHR asType,
		VkAccelerationStructureGeometryKHR& asGeometry,
		VkAccelerationStructureBuildRangeInfoKHR& asBuildRangeInfo,
		VkBuildAccelerationStructureFlagsKHR asFlags,
		AccelerationStructure& accStruct) {

		VkAccelerationStructureBuildGeometryInfoKHR asBuildGeometryInfo{};
		asBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		asBuildGeometryInfo.type = asType;
		asBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		asBuildGeometryInfo.flags = asFlags;
		asBuildGeometryInfo.geometryCount = 1;
		asBuildGeometryInfo.pGeometries = &asGeometry;


		VkAccelerationStructureBuildSizesInfoKHR asBuildSizesInfo{};
		asBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

		vkGetAccelerationStructureBuildSizesKHR(
			device.getDevice(),
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&asBuildGeometryInfo,
			&asBuildRangeInfo.primitiveCount,
			&asBuildSizesInfo);

		uint32_t scratchOffsetAlignment = device.accelStructProperties.minAccelerationStructureScratchOffsetAlignment;

		LthBuffer scratchBuffer{
			device,
			asBuildSizesInfo.buildScratchSize,
			1,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			scratchOffsetAlignment
		};

		VkBufferDeviceAddressInfo scratchBufferDeviceAddressInfo{
			.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
			.pNext = nullptr,
			.buffer = scratchBuffer.getBuffer(),
		};

		auto scratchBufferDeviceAddress = vkGetBufferDeviceAddress(device.getDevice(), &scratchBufferDeviceAddressInfo);

		accStruct.buffer = std::make_unique<LthBuffer>(
			device,
			asBuildSizesInfo.accelerationStructureSize,
			1,
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);
		VkAccelerationStructureCreateInfoKHR asCreateInfo{};
		asCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		asCreateInfo.buffer = accStruct.buffer->getBuffer();
		asCreateInfo.size = asBuildSizesInfo.accelerationStructureSize;
		asCreateInfo.type = asType;

		vkCreateAccelerationStructureKHR(device.getDevice(), &asCreateInfo, nullptr, &accStruct.handle);

		asBuildGeometryInfo.dstAccelerationStructure = accStruct.handle;
		asBuildGeometryInfo.scratchData.deviceAddress = scratchBufferDeviceAddress;

		device.buildAccelerationStructure(asBuildGeometryInfo, asBuildRangeInfo);

		VkAccelerationStructureDeviceAddressInfoKHR asDeviceAddressInfo{};
		asDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		asDeviceAddressInfo.accelerationStructure = accStruct.handle;

		accStruct.deviceAddress =
			vkGetAccelerationStructureDeviceAddressKHR(device.getDevice(), &asDeviceAddressInfo);
	}
}
