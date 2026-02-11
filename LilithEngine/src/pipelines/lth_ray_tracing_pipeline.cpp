#include "lth_ray_tracing_pipeline.hpp"
#include "../lth_global_info.hpp"


#include <slang/slang-com-ptr.h>
#include <slang/slang.h>
#include <stdexcept>
#include <iostream>

namespace lth {

	LthRayTracingPipeline::LthRayTracingPipeline(
		LthDevice& device,
		const VkPipelineLayout& pipelineLayout,
		LthShaderCompiler& shaderCompiler,
		const LthRayTracingPipelineFilePaths& rayTracingFilePaths) : LthPipeline(device, pipelineLayout, shaderCompiler),
		rayTracingFilePaths(rayTracingFilePaths) {
		slangSessions.push_back(shaderCompiler.createDefaultSlangSession());
		createRayTracingPipeline(pipelineLayout, rayTracingFilePaths);
	}


	LthRayTracingPipeline::~LthRayTracingPipeline() {
		clearPipeline();
	}

	void LthRayTracingPipeline::reloadPipeline() {
		clearPipeline();
		createRayTracingPipeline(lthPipelineLayout, rayTracingFilePaths);
	}

	void LthRayTracingPipeline::clearPipeline() {
		vkDestroyShaderModule(lthDevice.getDevice(), rayGenShaderModule, nullptr);
		vkDestroyShaderModule(lthDevice.getDevice(), missShaderModule, nullptr);
		vkDestroyShaderModule(lthDevice.getDevice(), chitShaderModule, nullptr);
		vkDestroyShaderModule(lthDevice.getDevice(), anyHitShaderModule, nullptr);
		vkDestroyPipeline(lthDevice.getDevice(), rayTracingPipeline, nullptr);
	}

	void LthRayTracingPipeline::bind(VkCommandBuffer commandBuffer) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rayTracingPipeline);
	}

	void LthRayTracingPipeline::trace(VkCommandBuffer commandBuffer) {
		const VkExtent2D& size = lthDevice.getWindow().getExtent();
		vkCmdTraceRaysKHR(commandBuffer, &rayGenRegion, &missRegion, &chitRegion, &callableRegion, size.width, size.height, 1);

	}

	void LthRayTracingPipeline::createRayTracingPipeline(
		const VkPipelineLayout& pipelineLayout,
		const LthRayTracingPipelineFilePaths& rayTracingFilePaths) {

		assert(pipelineLayout != VK_NULL_HANDLE &&
			"Cannot create ray tracing pipeline: no pipelineLayout provided in configInfo.");

		if (rayTracingFilePaths.rayGenFilePath.ends_with(".slang")) {

		}
		
		auto anyHitCode = readFile(rayTracingFilePaths.anyHitFilePath);
		createShaderModule(anyHitCode, &anyHitShaderModule);

		rayGenShaderModule = lthShaderCompiler.createShaderModule(rayTracingFilePaths.rayGenFilePath, &slangSessions[0], "main");
		missShaderModule = lthShaderCompiler.createShaderModule(rayTracingFilePaths.missFilePath, &slangSessions[0], "main");
		chitShaderModule = lthShaderCompiler.createShaderModule(rayTracingFilePaths.chitFilePath, &slangSessions[0], "main");

		std::vector<VkPipelineShaderStageCreateInfo> shaderStageInfos(4);
		shaderStageInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfos[0].stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		shaderStageInfos[0].module = rayGenShaderModule;
		shaderStageInfos[0].pName = "main";
		shaderStageInfos[0].flags = 0;
		shaderStageInfos[0].pNext = nullptr;
		shaderStageInfos[0].pSpecializationInfo = nullptr;

		shaderStageInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfos[1].stage = VK_SHADER_STAGE_MISS_BIT_KHR;
		shaderStageInfos[1].module = missShaderModule;
		shaderStageInfos[1].pName = "main";
		shaderStageInfos[1].flags = 0;
		shaderStageInfos[1].pNext = nullptr;
		shaderStageInfos[1].pSpecializationInfo = nullptr;

		shaderStageInfos[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfos[2].stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		shaderStageInfos[2].module = chitShaderModule;
		shaderStageInfos[2].pName = "main";
		shaderStageInfos[2].flags = 0;
		shaderStageInfos[2].pNext = nullptr;
		shaderStageInfos[2].pSpecializationInfo = nullptr;

		shaderStageInfos[3].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfos[3].stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
		shaderStageInfos[3].module = anyHitShaderModule;
		shaderStageInfos[3].pName = "main";
		shaderStageInfos[3].flags = 0;
		shaderStageInfos[3].pNext = nullptr;
		shaderStageInfos[3].pSpecializationInfo = nullptr;

		VkRayTracingShaderGroupCreateInfoKHR group{ VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR };
		group.generalShader = VK_SHADER_UNUSED_KHR;
		group.anyHitShader = VK_SHADER_UNUSED_KHR;
		group.closestHitShader = VK_SHADER_UNUSED_KHR;
		group.intersectionShader = VK_SHADER_UNUSED_KHR;

		std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroupInfos(4);

		// RayGen
		group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		group.generalShader = 0;
		shaderGroupInfos[0] = group;

		// Miss
		group.generalShader = 1;
		shaderGroupInfos[1] = group;

		// Chit
		group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		group.generalShader = VK_SHADER_UNUSED_KHR;
		group.closestHitShader = 2;
		shaderGroupInfos[2] = group;

		// AnyHit
		group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		group.closestHitShader = VK_SHADER_UNUSED_KHR;
		group.anyHitShader = 3;
		shaderGroupInfos[3] = group;

		VkRayTracingPipelineCreateInfoKHR rtPipelineCreateInfo {};
		rtPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		rtPipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStageInfos.size());
		rtPipelineCreateInfo.pStages = shaderStageInfos.data();
		rtPipelineCreateInfo.groupCount = static_cast<uint32_t>(shaderGroupInfos.size());
		rtPipelineCreateInfo.pGroups = shaderGroupInfos.data();
		rtPipelineCreateInfo.maxPipelineRayRecursionDepth = std::max(MAX_RAY_RECURSION_DEPTH, lthDevice.rayTracingProperties.maxRayRecursionDepth);
		rtPipelineCreateInfo.layout = pipelineLayout;
		vkCreateRayTracingPipelinesKHR(lthDevice.getDevice(), VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rtPipelineCreateInfo, nullptr, &rayTracingPipeline);
		
	
		createShaderBindingTable(rtPipelineCreateInfo);
	}


	void LthRayTracingPipeline::createShaderBindingTable(VkRayTracingPipelineCreateInfoKHR& rtPipelineCreateInfo) {

		uint32_t handleSize = lthDevice.rayTracingProperties.shaderGroupHandleSize;
		uint32_t handleAlignment = lthDevice.rayTracingProperties.shaderGroupHandleAlignment;
		uint32_t baseAlignment = lthDevice.rayTracingProperties.shaderGroupBaseAlignment;
		uint32_t groupCount = rtPipelineCreateInfo.groupCount;

		size_t dataSize = handleSize * groupCount;
		shaderHandles.resize(dataSize);
		vkGetRayTracingShaderGroupHandlesKHR(lthDevice.getDevice(), rayTracingPipeline, 0, groupCount, dataSize, shaderHandles.data());
		
		uint32_t shaderSize = LthBuffer::getAlignment(handleSize, handleAlignment);

		// Ensure each region starts at a baseAlignment boundary
		uint32_t raygenOffset = 0;
		uint32_t missOffset = LthBuffer::getAlignment(raygenOffset + shaderSize, baseAlignment);
		uint32_t chitOffset = LthBuffer::getAlignment(missOffset + shaderSize, baseAlignment);
		uint32_t anyHitOffset = LthBuffer::getAlignment(chitOffset + shaderSize, baseAlignment);

		size_t bufferSize = anyHitOffset + shaderSize;

		sbtBuffer = std::make_unique<LthBuffer>(
			lthDevice,
			bufferSize,
			1,
			VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			);

		VkBufferDeviceAddressInfo sbtBufferDeviceAddressInfo{
			.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
			.pNext = nullptr,
			.buffer = sbtBuffer->getBuffer(),
		};
		auto sbtBufferDeviceAddress = vkGetBufferDeviceAddress(lthDevice.getDevice(), &sbtBufferDeviceAddressInfo);


		sbtBuffer->map();
		sbtBuffer->writeToBuffer(shaderHandles.data() + 0 * handleSize, handleSize, raygenOffset);
		rayGenRegion.deviceAddress = sbtBufferDeviceAddress + raygenOffset;
		rayGenRegion.stride = shaderSize;
		rayGenRegion.size = shaderSize;

		sbtBuffer->writeToBuffer(shaderHandles.data() + 1 * handleSize, handleSize, missOffset);
		missRegion.deviceAddress = sbtBufferDeviceAddress + missOffset;
		missRegion.stride = shaderSize;
		missRegion.size = shaderSize;

		sbtBuffer->writeToBuffer(shaderHandles.data() + 2 * handleSize, handleSize, chitOffset);
		chitRegion.deviceAddress = sbtBufferDeviceAddress + chitOffset;
		chitRegion.stride = shaderSize;
		chitRegion.size = shaderSize;

		sbtBuffer->writeToBuffer(shaderHandles.data() + 3 * handleSize, handleSize, anyHitOffset);
		anyHitGenRegion.deviceAddress = sbtBufferDeviceAddress + anyHitOffset;
		anyHitGenRegion.stride = shaderSize;
		anyHitGenRegion.size = shaderSize;
	}
}