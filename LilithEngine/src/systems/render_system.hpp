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

	class RenderSystem {
	public:

		RenderSystem(LthDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
		~RenderSystem();

		RenderSystem(const RenderSystem&) = delete;
		RenderSystem& operator=(const RenderSystem&) = delete;
		
		void renderGameObjects(FrameInfo &frameInfo);
	private:
		void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
		void createPipeline(VkRenderPass renderPass);

		LthDevice &lthDevice;
		std::unique_ptr<LthPipeline> lthPipeline;
		VkPipelineLayout pipelineLayout;
	};
}

#endif