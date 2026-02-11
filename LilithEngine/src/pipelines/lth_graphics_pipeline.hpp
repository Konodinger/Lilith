#ifndef __LTH_GRAPHICS_PIPELINE_HPP__
#define __LTH_GRAPHICS_PIPELINE_HPP__

#include "lth_pipeline.hpp"

#include <string>
#include <vector>

namespace lth {

	struct LthGraphicsPipelineConfigInfo {
		LthGraphicsPipelineConfigInfo() = default;
		LthGraphicsPipelineConfigInfo(const LthGraphicsPipelineConfigInfo&) = delete;
		LthGraphicsPipelineConfigInfo& operator=(const LthGraphicsPipelineConfigInfo&) = delete;

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

	struct LthGraphicsPipelineFilePaths {
		std::string vertexFilePath = "";
		std::string fragmentFilePath = "";
	};

	class LthGraphicsPipeline : public LthPipeline {
	public:
		LthGraphicsPipeline(
			LthDevice& device,
			const LthGraphicsPipelineConfigInfo& configInfo,
			LthShaderCompiler& shaderCompiler,
			const LthGraphicsPipelineFilePaths& graphicsFilePath);
		~LthGraphicsPipeline() override;

		LthGraphicsPipeline() = default;
		LthGraphicsPipeline(const LthGraphicsPipeline&) = delete;
		LthGraphicsPipeline& operator=(const LthGraphicsPipeline&) = delete;
		void clearPipeline() override;
		void reloadPipeline() override;

		static void defaultGraphicsPipelineConfigInfo(LthGraphicsPipelineConfigInfo& configInfo);
		static void enableAlphaBlending(LthGraphicsPipelineConfigInfo& configInfo);

		void bind(VkCommandBuffer commandBuffer) override;
	private:
		//static std::vector<char> readFile(const std::string& filePath);
		void createGraphicsPipeline(
			const LthGraphicsPipelineConfigInfo& configInfo,
			const LthGraphicsPipelineFilePaths& graphicsFilePath);

		//void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

		//LthDevice& lthDevice;
		VkPipeline graphicsPipeline;
		const LthGraphicsPipelineConfigInfo& configInfo;
		const LthGraphicsPipelineFilePaths& graphicsFilePath;

		VkShaderModule vertShaderModule;
		VkShaderModule fragShaderModule;
	};
}

#endif