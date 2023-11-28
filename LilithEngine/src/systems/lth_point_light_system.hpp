#ifndef __POINT_LIGHT_SYSTEM_HPP__
#define __POINT_LIGHT_SYSTEM_HPP__

#include "../lth_pipeline.hpp"
#include "../lth_device.hpp"
#include "../lth_camera.hpp"
#include "../lth_frame_info.hpp"
#include "../gameObjects/lth_game_object.hpp"

#include <memory>
#include <vector>

namespace lth {

	class LthPointLightSystem {
	public:

		LthPointLightSystem(LthDevice& device, VkRenderPass renderPass, DescriptorSetLayouts& setLayouts);
		~LthPointLightSystem();

		LthPointLightSystem(const LthPointLightSystem&) = delete;
		LthPointLightSystem& operator=(const LthPointLightSystem&) = delete;
		
		void update(FrameInfo& frameInfo, GlobalUBO& ubo);
		void render(FrameInfo &frameInfo);
	private:
		void createPipelineLayout(DescriptorSetLayouts& setLayouts);
		void createPipeline(VkRenderPass renderPass);

		LthDevice &lthDevice;
		std::unique_ptr<LthPipeline> lthPipeline;
		VkPipelineLayout pipelineLayout;
	};
}

#endif