#include "lth_point_light_system.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <cassert>
#include <stdexcept>
#include <array>
#include <iostream>
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
		: lthDevice{ device } {
		createPipelineLayout(setLayouts);
		createPipeline(renderPass);
	}

	LthPointLightSystem::~LthPointLightSystem() {
		vkDestroyPipelineLayout(lthDevice.device(), pipelineLayout, nullptr);
	}

	void LthPointLightSystem::createPipelineLayout(DescriptorSetLayouts& setLayouts) {

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PointLightPushConstants);

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ setLayouts.globalSetLayout->getDescriptorSetLayout(),
																setLayouts.gameObjectSetLayout->getDescriptorSetLayout()};

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		if (vkCreatePipelineLayout(lthDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create pipeline layout!");
		}
	}

	void LthPointLightSystem::createPipeline(VkRenderPass renderPass) {
		assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout!");

		LthGraphicsPipelineConfigInfo pipelineConfig{};
		LthGraphicsPipeline::defaultGraphicsPipelineConfigInfo(pipelineConfig);
		LthGraphicsPipeline::enableAlphaBlending(pipelineConfig);
		pipelineConfig.bindingDescriptions.clear();
		pipelineConfig.attributeDescriptions.clear();
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		pipelineConfig.multisampleInfo.rasterizationSamples = lthDevice.getMsaaSamples();
		lthGraphicsPipeline = std::make_unique<LthGraphicsPipeline>(
			lthDevice,
			pipelineConfig,
			"shaders/pointLight.vert.spv",
			"shaders/pointLight.frag.spv");
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

		lthGraphicsPipeline->bind(frameInfo.commandBuffer);

		vkCmdBindDescriptorSets(
			frameInfo.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
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
				frameInfo.commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(PointLightPushConstants),
				&push
			);

			vkCmdBindDescriptorSets(
				frameInfo.commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayout,
				1,
				1,
				&obj.gameObjectDescriptorSets[frameInfo.frameIndex],
				0,
				nullptr);


			vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
		}

	}
}