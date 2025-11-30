#ifndef __RAY_TRACING_SYSTEM_HPP__
#define __RAY_TRACING_SYSTEM_HPP__

#include "lth_system.hpp"
#include "../pipelines/lth_ray_tracing_pipeline.hpp"
#include "../lth_frame_info.hpp"

namespace lth {

	struct RayTracingPushConstantData {
		glm::mat4 modelMatrix{ 1.f };
		glm::mat4 normalMatrix{ 1.f };
	};

	class LthRayTracingSystem : public LthSystem {
		LthRayTracingPipelineFilePaths rayTracingFilePaths = { 
			.rayGenFilePath = SHADERSFOLDERPATH("rayTracing/standardRT.rgen"),
			.anyHitFilePath = SHADERSFOLDERPATH("rayTracing/standardRT.rahit"),
			.chitFilePath = SHADERSFOLDERPATH("rayTracing/standardRT.rchit"),
			.missFilePath = SHADERSFOLDERPATH("rayTracing/standardRT.rmiss") };
	public:
		LthRayTracingSystem(LthDevice& device,
			VkRenderPass renderPass,
			DescriptorSetLayouts& setLayouts);
		~LthRayTracingSystem() {
			vkDestroyPipelineLayout(lthDevice.getDevice(), rayTracingPipelineLayout, nullptr);
		}

		void createPipeline(VkRenderPass renderPass);
		void trace(FrameInfo& frameInfo, VkDescriptorSet& computeDescriptorSet);

		bool activateTrace = true;
	protected:
		std::unique_ptr<LthRayTracingPipeline> lthRayTracingPipeline;
		VkPipelineLayout rayTracingPipelineLayout = 0;
	};
}

#endif