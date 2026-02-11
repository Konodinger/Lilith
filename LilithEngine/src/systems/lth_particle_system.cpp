#include "lth_particle_system.hpp"

#include <cassert>
#include <random>
#include "../lth_utils.hpp"

namespace lth {

	LthParticleSystem::LthParticleSystem(
		LthDevice& device,
		LthShaderCompiler& shaderCompiler,
		VkRenderPass renderPass,
		DescriptorSetLayouts& setLayouts,
		std::vector<std::unique_ptr<LthBuffer>>& cboBuffers)
		: LthGraphicsSystem(device, shaderCompiler, renderPass, setLayouts) {
		storageBuffers = std::move(cboBuffers);
		createPipelineLayout(&graphicsPipelineLayout);
		createPipeline(renderPass);

		std::vector<VkDescriptorSetLayout> computeDescriptorSetLayouts{ setLayouts.globalSetLayout->getDescriptorSetLayout(),
																		setLayouts.computeSetLayout->getDescriptorSetLayout() };
		createPipelineLayout(&computePipelineLayout, computeDescriptorSetLayouts);
		createComputePipeline(renderPass);
	}

	LthParticleSystem::~LthParticleSystem() {
		vkDestroyPipelineLayout(lthDevice.getDevice(), computePipelineLayout, nullptr);
	}

	void LthParticleSystem::createPipeline(VkRenderPass renderPass) {
		//To complete
		assert(graphicsPipelineLayout != nullptr && "Cannot create pipeline before pipeline layout!");

		LthGraphicsPipelineConfigInfo pipelineConfig{};
		LthGraphicsPipeline::defaultGraphicsPipelineConfigInfo(pipelineConfig);
		pipelineConfig.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		
		pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		pipelineConfig.rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		
		pipelineConfig.depthStencilInfo.depthTestEnable = VK_FALSE;
		pipelineConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;

		pipelineConfig.colorBlendAttachment.blendEnable = VK_TRUE;
		pipelineConfig.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

		pipelineConfig.bindingDescriptions = Particle::getBindingDescriptions();
		pipelineConfig.attributeDescriptions = Particle::getAttributeDescriptions();
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = graphicsPipelineLayout;
		pipelineConfig.multisampleInfo.rasterizationSamples = lthDevice.getMsaaSamples();
		lthGraphicsPipeline = std::make_unique<LthGraphicsPipeline>(
			lthDevice,
			pipelineConfig,
			lthShaderCompiler,
			particleRenderFilePaths);
	}

	void LthParticleSystem::createComputePipeline(VkRenderPass renderPass) {
		assert(computePipelineLayout != nullptr && "Cannot create pipeline before pipeline layout!");

		lthComputePipeline = std::make_unique<LthComputePipeline>(
			lthDevice,
			computePipelineLayout,
			lthShaderCompiler,
			computeShaderSpvPath);
	}

	bool LthParticleSystem::checkForPipelineUpdates() {
		bool updates = lthGraphicsPipeline->checkForUpdatesAndReload();
		updates |= lthComputePipeline->checkForUpdatesAndReload();
		return updates;
	}


	void *LthParticleSystem::initialStorageBufferData() {
		std::default_random_engine rndEngine((unsigned)time(nullptr));
		std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

		// Initial particle positions on a circle
		std::vector<Particle>* particles = new std::vector<Particle>(PARTICLE_COUNT);
		for (auto& particle : *particles) {
			float r = 0.25f * sqrt(rndDist(rndEngine));
			float theta = rndDist(rndEngine) * TWO_PI;
			float x = r * cos(theta) * HEIGHT / WIDTH;
			float y = r * sin(theta);
			particle.position = glm::vec2(x, y);
			particle.velocity = glm::normalize(glm::vec2(x, y)) * 0.00025f;
			particle.color = glm::vec4(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine), 1.0f);
		}

		for (int i = 0; i < 10; ++i) {
			Particle particle = (*particles)[i];
		}

		return (*particles).data();
	}

	void LthParticleSystem::createStorageBuffer(LthDevice& device, std::vector<std::unique_ptr<LthBuffer>>& storageBuffers) {

		LthBuffer stagingBuffer{
			device,
			sizeof(Particle),
			PARTICLE_COUNT,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};

		stagingBuffer.map();
		stagingBuffer.writeToBuffer(initialStorageBufferData());
		stagingBuffer.unmap();

		storageBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < storageBuffers.size(); ++i) {
			storageBuffers[i] = std::make_unique<LthBuffer>(
				device,
				sizeof(Particle),
				PARTICLE_COUNT,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			device.copyBuffer(stagingBuffer.getBuffer(), storageBuffers[i]->getBuffer(), stagingBuffer.getBufferSize());
		}
	}

	void LthParticleSystem::dispatch(FrameInfo& frameInfo, VkDescriptorSet& computeDescriptorSet) {

		VkCommandBuffer commandBuffer = frameInfo.computeCommandBuffer;
		lthComputePipeline->bind(commandBuffer);

		//vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &frameInfo.globalDescriptorSet, 0, 0);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 1, 1, &computeDescriptorSet, 0, 0);

		vkCmdDispatch(commandBuffer, PARTICLE_COUNT / 256, 1, 1);
	}

	void LthParticleSystem::render(FrameInfo& frameInfo) {
		if (!activateRender) return;

		VkCommandBuffer commandBuffer = frameInfo.graphicsCommandBuffer;

		lthGraphicsPipeline->bind(commandBuffer);

		/*vkCmdBindDescriptorSets(
			frameInfo.graphicsCommandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			graphicsPipelineLayout,
			0,
			1,
			&frameInfo.globalDescriptorSet,
			0,
			nullptr);*/
		VkBuffer buffers[] = { storageBuffers[frameInfo.frameIndex]->getBuffer() };
		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
		vkCmdDraw(commandBuffer, PARTICLE_COUNT, 1, 0, 0);
	}
}