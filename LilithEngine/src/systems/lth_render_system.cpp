#include "lth_render_system.hpp"

#include <cassert>
#include <array>

namespace lth {
	
	struct SimplePushConstantData {
		glm::mat4 modelMatrix{ 1.f };
		glm::mat4 normalMatrix{ 1.f };
	};

	LthRenderSystem::LthRenderSystem(
		LthDevice& device,
		VkRenderPass renderPass,
		DescriptorSetLayouts& setLayouts)
		: LthGraphicsSystem(device, renderPass, setLayouts) {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(SimplePushConstantData);

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ setLayouts.globalSetLayout->getDescriptorSetLayout(),
																setLayouts.gameObjectSetLayout->getDescriptorSetLayout() };
		createPipelineLayout(&graphicsPipelineLayout,
			descriptorSetLayouts,
			{ pushConstantRange });
		createPipeline(renderPass);
	}

	void LthRenderSystem::createPipeline(VkRenderPass renderPass) {
		assert(graphicsPipelineLayout != nullptr && "Cannot create pipeline before pipeline layout!");

		LthGraphicsPipelineConfigInfo pipelineConfig{};
		LthGraphicsPipeline::defaultGraphicsPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = graphicsPipelineLayout;
		pipelineConfig.multisampleInfo.rasterizationSamples = lthDevice.getMsaaSamples();
		lthGraphicsPipeline = std::make_unique<LthGraphicsPipeline>(
			lthDevice,
			pipelineConfig,
			vertexShaderSpvPath,
			fragmentShaderSpvPath);
	}

	void LthRenderSystem::render(FrameInfo& frameInfo) {
		if (!activateRender) return;

		lthGraphicsPipeline->bind(frameInfo.graphicsCommandBuffer);

		vkCmdBindDescriptorSets(
			frameInfo.graphicsCommandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			graphicsPipelineLayout,
			0,
			1,
			&frameInfo.globalDescriptorSet,
			0,
			nullptr);

		for (auto& keyValue : frameInfo.scene.getInstanceArray()) {
			auto& obj = frameInfo.scene.gameObject(keyValue.first);
			auto& model = frameInfo.scene.model(keyValue.second);

			SimplePushConstantData push{};
			push.modelMatrix = obj->transform.modelMatrix();
			push.normalMatrix = obj->transform.normalMatrix();

			vkCmdPushConstants(
				frameInfo.graphicsCommandBuffer,
				graphicsPipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(SimplePushConstantData),
				&push);

			vkCmdBindDescriptorSets(
				frameInfo.graphicsCommandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				graphicsPipelineLayout,
				1,
				1,
				&obj->gameObjectDescriptorSets[frameInfo.frameIndex],
				0,
				nullptr);

			model->bind(frameInfo.graphicsCommandBuffer);
			model->draw(frameInfo.graphicsCommandBuffer);
		}
	}
}