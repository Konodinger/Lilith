#ifndef __PARTICLE_SYSTEM_HPP__
#define __PARTICLE_SYSTEM_HPP__

#include "lth_graphics_system.hpp"
#include "../lth_global_info.hpp"
#include "../pipelines/lth_compute_pipeline.hpp"
#include "../lth_global_info.hpp"

#include <memory>
#include <vector>

namespace lth {

	struct Particle {
		glm::vec2 position;
		glm::vec2 velocity;
		glm::vec4 color;

		static std::vector<VkVertexInputBindingDescription> getBindingDescriptions() {
			std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescriptions[0].binding = 0;
			bindingDescriptions[0].stride = sizeof(Particle);
			bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescriptions;
		}

		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
			std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2, {});

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Particle, position);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Particle, color);

			return attributeDescriptions;
		}
	};

	static constexpr uint32_t PARTICLE_COUNT = 1024;

	class LthParticleSystem : LthGraphicsSystem {
		inline static std::string vertexShaderSpvPath = SHADERSFOLDERPATH("particle.vert.spv");
		inline static std::string fragmentShaderSpvPath = SHADERSFOLDERPATH("particle.frag.spv");
		inline static std::string computeShaderSpvPath = SHADERSFOLDERPATH("particle.comp.spv");

	public:

		LthParticleSystem(LthDevice& device,
						VkRenderPass renderPass,
						DescriptorSetLayouts& setLayouts,
						std::vector<std::unique_ptr<LthBuffer>>& cboBuffers);
		~LthParticleSystem();

		LthParticleSystem(const LthParticleSystem&) = delete;
		LthParticleSystem& operator=(const LthParticleSystem&) = delete;

		void dispatch(FrameInfo& frameInfo, VkDescriptorSet computeDescriptorSet);
		void render(FrameInfo& frameInfo);

		static void createStorageBuffer(LthDevice&, std::vector<std::unique_ptr<LthBuffer>>&);

	private:
		//void createPipelineLayout(DescriptorSetLayouts& setLayouts);
		void createPipeline(VkRenderPass renderPass);
		void createComputePipeline(VkRenderPass renderPass);

		static void* initialStorageBufferData();

		std::unique_ptr<LthComputePipeline> lthComputePipeline;
		VkPipelineLayout computePipelineLayout;
		std::vector<std::unique_ptr<LthBuffer>> storageBuffers;
	};
}

#endif