#ifndef __GRAPHICS_SYSTEM_HPP__
#define __GRAPHICS_SYSTEM_HPP__

#include "lth_system.hpp"
#include "../pipelines/lth_graphics_pipeline.hpp"
#include "../lth_frame_info.hpp"

namespace lth {

	class LthGraphicsSystem : public LthSystem {
	public:
		LthGraphicsSystem(LthDevice& device,
			LthShaderCompiler& shaderCompiler,
			VkRenderPass renderPass,
			DescriptorSetLayouts& setLayouts) : LthSystem(device, shaderCompiler, renderPass, setLayouts) {};
		~LthGraphicsSystem() {
			vkDestroyPipelineLayout(lthDevice.getDevice(), graphicsPipelineLayout, nullptr);
		}

		virtual void createPipeline(VkRenderPass renderPass) = 0;
		virtual bool checkForPipelineUpdates() override { return lthGraphicsPipeline->checkForUpdatesAndReload(); }
		virtual void render(FrameInfo& frameInfo) = 0;
		
		bool activateRender = true;
	protected:
		std::unique_ptr<LthGraphicsPipeline> lthGraphicsPipeline;
		VkPipelineLayout graphicsPipelineLayout = 0;

	};
}

#endif