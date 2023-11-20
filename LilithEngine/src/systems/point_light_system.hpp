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

	class PointLightSystem {
	public:

		PointLightSystem(LthDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
		~PointLightSystem();

		PointLightSystem(const PointLightSystem&) = delete;
		PointLightSystem& operator=(const PointLightSystem&) = delete;
		
		void update(FrameInfo& frameInfo, GlobalUBO& ubo);
		void render(FrameInfo &frameInfo);
	private:
		void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
		void createPipeline(VkRenderPass renderPass);

		LthDevice &lthDevice;
		std::unique_ptr<LthPipeline> lthPipeline;
		VkPipelineLayout pipelineLayout;
	};
}

#endif