#ifndef __LTH_RAY_TRACING_PIPELINE_HPP__
#define __LTH_RAY_TRACING_PIPELINE_HPP__

#include "lth_pipeline.hpp"
#include "../lth_buffer.hpp"

#include <string>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

namespace lth {

	struct LthRayTracingPipelineConfigInfo {
		LthRayTracingPipelineConfigInfo() = default;
		LthRayTracingPipelineConfigInfo(const LthRayTracingPipelineConfigInfo&) = delete;
		LthRayTracingPipelineConfigInfo& operator=(const LthRayTracingPipelineConfigInfo&) = delete;

		VkPipelineLayout pipelineLayout = nullptr;
	};

	struct LthRayTracingPipelineFilePaths {
		std::string rayGenFilePath = "";
		std::string missFilePath = "";
		std::string chitFilePath = "";
		std::string anyHitFilePath = "";
	};

	class LthRayTracingPipeline : public LthPipeline {
	public:
		LthRayTracingPipeline(
			LthDevice& device,
			const VkPipelineLayout& pipelineLayout,
			LthShaderCompiler& shaderCompiler,
			const LthRayTracingPipelineFilePaths& rayTracingFilePaths);
		~LthRayTracingPipeline() override;

		LthRayTracingPipeline() = default;
		LthRayTracingPipeline(const LthRayTracingPipeline&) = delete;
		LthRayTracingPipeline& operator=(const LthRayTracingPipeline&) = delete;

		void clearPipeline() override;
		void reloadPipeline() override;

		void bind(VkCommandBuffer commandBuffer) override;
		void trace(VkCommandBuffer commandBuffer);
	private:
		void createRayTracingPipeline(
			const VkPipelineLayout& pipelineLayout,
			const LthRayTracingPipelineFilePaths& rayTracingFilePaths);
		void createShaderBindingTable(VkRayTracingPipelineCreateInfoKHR& rtPipelineCreateInfo);

		VkPipeline rayTracingPipeline;
		const LthRayTracingPipelineFilePaths& rayTracingFilePaths;

		VkShaderModule rayGenShaderModule;
		VkShaderModule anyHitShaderModule;
		VkShaderModule chitShaderModule;
		VkShaderModule missShaderModule;
		//VkShaderModule callableShaderModule;

		std::vector<uint8_t> shaderHandles{};
		std::unique_ptr<LthBuffer> sbtBuffer;
		VkStridedDeviceAddressRegionKHR rayGenRegion{};
		VkStridedDeviceAddressRegionKHR missRegion{};
		VkStridedDeviceAddressRegionKHR chitRegion{};
		VkStridedDeviceAddressRegionKHR anyHitGenRegion{};
		VkStridedDeviceAddressRegionKHR callableRegion{};
		
	};
}

#endif