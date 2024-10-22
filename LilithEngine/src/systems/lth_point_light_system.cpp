#include "lth_point_light_system.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <cassert>
#include <map>

namespace lth {

	struct PointLightPushConstants {
		glm::vec4 position{};
		glm::vec4 color{};
		float radius;
	};

	LthPointLightSystem::LthPointLightSystem(
		LthDevice& device,
		VkRenderPass renderPass,
		DescriptorSetLayouts& setLayouts)
		: LthGraphicsSystem(device, renderPass, setLayouts) {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PointLightPushConstants);

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ setLayouts.globalSetLayout->getDescriptorSetLayout(),
																setLayouts.gameObjectSetLayout->getDescriptorSetLayout() };
		createPipelineLayout(&graphicsPipelineLayout,
			descriptorSetLayouts,
			{ pushConstantRange });
		createPipeline(renderPass);
	}

	void LthPointLightSystem::createPipeline(VkRenderPass renderPass) {
		assert(graphicsPipelineLayout != nullptr && "Cannot create pipeline before pipeline layout!");

		LthGraphicsPipelineConfigInfo pipelineConfig{};
		LthGraphicsPipeline::defaultGraphicsPipelineConfigInfo(pipelineConfig);
		LthGraphicsPipeline::enableAlphaBlending(pipelineConfig);
		pipelineConfig.bindingDescriptions.clear();
		pipelineConfig.attributeDescriptions.clear();
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = graphicsPipelineLayout;
		pipelineConfig.multisampleInfo.rasterizationSamples = lthDevice.getMsaaSamples();
		lthGraphicsPipeline = std::make_unique<LthGraphicsPipeline>(
			lthDevice,
			pipelineConfig,
			vertexShaderSpvPath,
			fragmentShaderSpvPath);
	}

	void LthPointLightSystem::update(FrameInfo& frameInfo, GlobalUBO& ubo) {
		int lightIndex = 0;
		for (auto& kv : frameInfo.gameObjects) {
			auto& obj = kv.second;
			if (obj.pointLight == nullptr) continue;

			assert(lightIndex < MAX_LIGHTS && "Point lights exceed maximum specified!");

			ubo.pointLights[lightIndex].position = obj.transform.getTranslation();
			ubo.pointLights[lightIndex].lightQuadraticAttenuation = obj.pointLight->lightQuadraticAttenuation;
			ubo.pointLights[lightIndex].color = glm::vec4(obj.color, obj.pointLight->lightIntensity);
			lightIndex += 1;

		}
		ubo.numLights = lightIndex;
	}

	void LthPointLightSystem::render(FrameInfo& frameInfo) {

		//Note: looping through every gameobject is inefficient and need to be readjusted.
		std::map<float, LthGameObject::id_t> sorted;
		for (auto& kv : frameInfo.gameObjects) {
			auto& obj = kv.second;
			if (obj.pointLight == nullptr) continue;
			auto offset = frameInfo.camera.getPosition() - obj.transform.getTranslation();
			float distSquared = glm::dot(offset, offset);
			sorted[distSquared] = obj.getId();
		}

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

		//Iterate form farthest to nearest light.
		for (auto it = sorted.rbegin(); it != sorted.rend(); ++it) {
			auto& obj = frameInfo.gameObjects.at(it->second);

			PointLightPushConstants push{};
			push.position = glm::vec4(obj.transform.getTranslation(), 1.f);
			push.color = glm::vec4(obj.color, obj.pointLight->lightIntensity);
			push.radius = obj.transform.getScale().x;

			vkCmdPushConstants(
				frameInfo.graphicsCommandBuffer,
				graphicsPipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(PointLightPushConstants),
				&push
			);

			vkCmdBindDescriptorSets(
				frameInfo.graphicsCommandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				graphicsPipelineLayout,
				1,
				1,
				&obj.gameObjectDescriptorSets[frameInfo.frameIndex],
				0,
				nullptr);

			vkCmdDraw(frameInfo.graphicsCommandBuffer, 6, 1, 0, 0);
		}

	}
}