#ifndef __RAY_TRACING_SYSTEM_HPP__
#define __RAY_TRACING_SYSTEM_HPP__

#include "lth_system.hpp"
#include "../pipelines/lth_graphics_pipeline.hpp"
#include "../lth_frame_info.hpp"

namespace lth {

	class LthRayTracingSystem : public LthSystem {
	public:
		LthRayTracingSystem(LthDevice& device,
			VkRenderPass renderPass,
			DescriptorSetLayouts& setLayouts) : LthSystem(device, renderPass, setLayouts) {};
		~LthRayTracingSystem() {
			vkDestroyPipelineLayout(lthDevice.device(), graphicsPipelineLayout, nullptr);
		}

		virtual void createPipeline(VkRenderPass renderPass) = 0;
		virtual void render(FrameInfo& frameInfo) = 0;
	protected:
		std::unique_ptr<LthGraphicsPipeline> lthGraphicsPipeline;
		VkPipelineLayout graphicsPipelineLayout = 0;
	};
}

#endif