#include "lth_ray_tracing_system.hpp"

namespace lth {

	LthRayTracingSystem::LthRayTracingSystem(
		LthDevice& device,
		LthShaderCompiler& shaderCompiler,
		VkRenderPass renderPass,
		DescriptorSetLayouts& setLayouts) : LthSystem(device, shaderCompiler, renderPass, setLayouts) {

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_ALL;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(RayTracingPushConstantData);

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ setLayouts.globalSetLayout->getDescriptorSetLayout(),
																setLayouts.rayTracingSetLayout->getDescriptorSetLayout() };
		createPipelineLayout(&rayTracingPipelineLayout, descriptorSetLayouts, { pushConstantRange });
		createPipeline(renderPass);
	}

	void LthRayTracingSystem::createPipeline(VkRenderPass renderPass) {
		assert(rayTracingPipelineLayout != nullptr && "Cannot create pipeline before pipeline layout!");
		lthRayTracingPipeline = std::make_unique<LthRayTracingPipeline>(
			lthDevice,
			rayTracingPipelineLayout,
			lthShaderCompiler,
			rayTracingFilePaths);
	}

	bool LthRayTracingSystem::checkForPipelineUpdates() {
		return lthRayTracingPipeline->checkForUpdatesAndReload();
	}

	void LthRayTracingSystem::trace(FrameInfo& frameInfo, VkDescriptorSet& computeDescriptorSet) {
		if (!activateTrace) return;


		lthRayTracingPipeline->bind(frameInfo.graphicsCommandBuffer);

		std::vector<VkDescriptorSet> descriptorSets = { frameInfo.globalDescriptorSet, computeDescriptorSet };

		vkCmdBindDescriptorSets(
			frameInfo.graphicsCommandBuffer,
			VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
			rayTracingPipelineLayout,
			0,
			2,
			descriptorSets.data(),
			0,
			nullptr);

		RayTracingPushConstantData push{};

		vkCmdPushConstants(
			frameInfo.graphicsCommandBuffer,
			rayTracingPipelineLayout,
			VK_SHADER_STAGE_ALL,
			0,
			sizeof(RayTracingPushConstantData),
			&push);

		lthRayTracingPipeline->trace(frameInfo.graphicsCommandBuffer);
	}
}