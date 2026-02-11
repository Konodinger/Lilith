#include "lth_compute_pipeline.hpp"

#include <fstream>
#include <stdexcept>
#include <iostream>
#include <cassert>

namespace lth {

	LthComputePipeline::LthComputePipeline(
		LthDevice& device,
		const VkPipelineLayout& pipelineLayout,
		LthShaderCompiler& shaderCompiler,
		const std::string& computeFilePath) : LthPipeline(device, pipelineLayout, shaderCompiler),
			computeFilePath(computeFilePath) {
		createComputePipeline(pipelineLayout, computeFilePath);
	}

	LthComputePipeline::~LthComputePipeline() {
		clearPipeline();
	}

	void LthComputePipeline::reloadPipeline() {
		clearPipeline();
		createComputePipeline(lthPipelineLayout, computeFilePath);
	}

	void LthComputePipeline::clearPipeline() {
		vkDestroyShaderModule(lthDevice.getDevice(), computeShaderModule, nullptr);
		vkDestroyPipeline(lthDevice.getDevice(), computePipeline, nullptr);
	}

	void LthComputePipeline::bind(VkCommandBuffer commandBuffer) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
	}

	void LthComputePipeline::defaultComputePipelineConfigInfo(LthComputePipelineConfigInfo& configInfo) {
		//...
	}

	void LthComputePipeline::createComputePipeline(
		const VkPipelineLayout& pipelineLayout,
		const std::string& computeFilePath) {

		//assert section

		auto computeCode = readFile(computeFilePath);
		createShaderModule(computeCode, &computeShaderModule);

		VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
		computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		computeShaderStageInfo.module = computeShaderModule;
		computeShaderStageInfo.pName = "main";

		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.pNext = nullptr;
		pipelineInfo.stage = computeShaderStageInfo;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		if (vkCreateComputePipelines(
			lthDevice.getDevice(),
			VK_NULL_HANDLE,
			1,
			&pipelineInfo,
			nullptr,
			&computePipeline) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create compute pipeline!");
		}
	}
}