#ifndef __RENDER_SYSTEM_HPP__
#define __RENDER_SYSTEM_HPP__

#include "../lth_pipeline.hpp"
#include "../lth_device.hpp"
#include "../lth_camera.hpp"
#include "../lth_frame_info.hpp"
#include "../gameObjects/lth_game_object.hpp"

#include <memory>
#include <vector>

#define VERTEXSHADERSPVPATH "shaders/vertexShader.vert.spv"
#define FRAGMENTSHADERSPVPATH "shaders/fragmentShader.frag.spv"

namespace lth {

	class LthRenderSystem {
	public:

		LthRenderSystem(LthDevice& device, VkRenderPass renderPass, DescriptorSetLayouts& setLayouts);
		~LthRenderSystem();

		LthRenderSystem(const LthRenderSystem&) = delete;
		LthRenderSystem& operator=(const LthRenderSystem&) = delete;
		
		void renderGameObjects(FrameInfo &frameInfo);
	private:
		void createPipelineLayout(DescriptorSetLayouts& setLayouts);
		void createPipeline(VkRenderPass renderPass);

		LthDevice &lthDevice;
		std::unique_ptr<LthPipeline> lthPipeline;
		VkPipelineLayout pipelineLayout;
	};
}

#endif