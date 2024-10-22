#ifndef __LTH_COMPUTE_PIPELINE_HPP__
#define __LTH_COMPUTE_PIPELINE_HPP__

#include "lth_pipeline.hpp"

#include <string>
#include <vector>

namespace lth {

	struct LthComputePipelineConfigInfo {
		LthComputePipelineConfigInfo() = default;
		LthComputePipelineConfigInfo(const LthComputePipelineConfigInfo&) = delete;
		LthComputePipelineConfigInfo& operator=(const LthComputePipelineConfigInfo&) = delete;

		VkPipelineLayout pipelineLayout = nullptr;
	};

	class LthComputePipeline : public LthPipeline {
	public:
		LthComputePipeline(
			LthDevice& device,
			VkPipelineLayout& pipelineLayout,
			const std::string& computeFilePath);
		~LthComputePipeline() override;

		LthComputePipeline() = default;
		LthComputePipeline(const LthComputePipeline&) = delete;
		LthComputePipeline& operator=(const LthComputePipeline&) = delete;

		static void defaultComputePipelineConfigInfo(LthComputePipelineConfigInfo& configInfo);

		void bind(VkCommandBuffer commandBuffer) override;
	private:
		void createComputePipeline(
			VkPipelineLayout& pipelineLayout,
			const std::string& computeFilePath);


		VkPipeline computePipeline;
		VkShaderModule computeShaderModule;
	};
}

#endif