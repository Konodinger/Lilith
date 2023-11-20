#ifndef __LTH_PIPELINE_HPP__
#define __LTH_PIPELINE_HPP__

#include "lth_device.hpp"

#include <string>
#include <vector>

namespace lth {

	struct PipelineConfigInfo {
		PipelineConfigInfo() = default;
		PipelineConfigInfo(const PipelineConfigInfo&) = delete;
		PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

		std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
		VkPipelineViewportStateCreateInfo viewportInfo;
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
		VkPipelineRasterizationStateCreateInfo rasterizationInfo;
		VkPipelineMultisampleStateCreateInfo multisampleInfo;
		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		VkPipelineColorBlendStateCreateInfo colorBlendInfo;
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
		std::vector<VkDynamicState> dynamicStateEnables;
		VkPipelineDynamicStateCreateInfo dynamicStateInfo;
		VkPipelineLayout pipelineLayout = nullptr;
		VkRenderPass renderPass = nullptr;
		uint32_t subpass = 0;
	};

	class LthPipeline {
	public:
		LthPipeline(
			LthDevice& device,
			const PipelineConfigInfo& configInfo,
			const std::string& vertFilePath,
			const std::string& fragFilePath);
		~LthPipeline();

		LthPipeline() = default;
		LthPipeline(const LthPipeline&) = delete;
		LthPipeline& operator=(const LthPipeline&) = delete;

		static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);
		static void enableAlphaBlending(PipelineConfigInfo& configInfo);

		void bind(VkCommandBuffer commandBuffer);
	private:
		static std::vector<char> readFile(const std::string& filePath);
		void createGraphicsPipeline(
			const PipelineConfigInfo& configInfo,
			const std::string& vertFilePath,
			const std::string& fragFilePath);
	
		void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

		LthDevice& lthDevice;
		VkPipeline graphicsPipeline;
		VkShaderModule vertShaderModule;
		VkShaderModule fragShaderModule;
	};
}

#endif