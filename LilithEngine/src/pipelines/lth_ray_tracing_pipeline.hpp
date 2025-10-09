#ifndef __LTH_RAY_TRACING_PIPELINE_HPP__
#define __LTH_RAY_TRACING_PIPELINE_HPP__

#include "lth_pipeline.hpp"

#include <string>
#include <vector>

namespace lth {

	struct LthRayTracingPipelineConfigInfo {
		LthRayTracingPipelineConfigInfo() = default;
		LthRayTracingPipelineConfigInfo(const LthRayTracingPipelineConfigInfo&) = delete;
		LthRayTracingPipelineConfigInfo& operator=(const LthRayTracingPipelineConfigInfo&) = delete;

		VkPipelineLayout pipelineLayout = nullptr;
	};

	struct LthRayTracingPipelineFilePaths {
		LthRayTracingPipelineFilePaths() = default;
		std::string rayGenFilePath = "";
		std::string aniHitFilePath = "";
		std::string chitFilePath = "";
		std::string missFilePath = "";
	};

	class LthRayTracingPipeline : public LthPipeline {
	public:
		LthRayTracingPipeline(
			LthDevice& device,
			VkPipelineLayout& pipelineLayout,
			const std::string& computeFilePath);
		~LthRayTracingPipeline() override;

		LthRayTracingPipeline() = default;
		LthRayTracingPipeline(const LthRayTracingPipeline&) = delete;
		LthRayTracingPipeline& operator=(const LthRayTracingPipeline&) = delete;

		static void defaultRayTracingPipelineConfigInfo(LthRayTracingPipelineConfigInfo& configInfo);

		void bind(VkCommandBuffer commandBuffer) override;
	private:
		void createRayTracingPipeline(
			VkPipelineLayout& pipelineLayout,
			const LthRayTracingPipelineFilePaths& rayTracingFilePaths);


		VkPipeline rayTracingPipeline;
		VkShaderModule rayTracingShaderModule;
	};
}

#endif