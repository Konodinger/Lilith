#ifndef __RENDER_SYSTEM_HPP__
#define __RENDER_SYSTEM_HPP__

#include "lth_graphics_system.hpp"
#include "../gameObjects/lth_game_object.hpp"
#include "../lth_global_info.hpp"

#include <memory>
#include <vector>

namespace lth {

	struct SimplePushConstantData {
		glm::mat4 modelMatrix{ 1.f };
		glm::mat4 normalMatrix{ 1.f };
	};

	class LthRenderSystem : public LthGraphicsSystem {
		inline static std::string vertexShaderSpvPath = SHADERSFOLDERPATH("standard.vert");
		inline static std::string fragmentShaderSpvPath = SHADERSFOLDERPATH("standard.frag");

	public:

		LthRenderSystem(LthDevice& device, VkRenderPass renderPass, DescriptorSetLayouts& setLayouts);
		//~LthRenderSystem();

		LthRenderSystem(const LthRenderSystem&) = delete;
		LthRenderSystem& operator=(const LthRenderSystem&) = delete;
		
		void render(FrameInfo &frameInfo);
	private:
		//void createPipelineLayout(DescriptorSetLayouts& setLayouts);
		void createPipeline(VkRenderPass renderPass);


		//LthDevice &lthDevice;
		//std::unique_ptr<LthGraphicsPipeline> lthGraphicsPipeline;
		//VkPipelineLayout pipelineLayout;
	};
}

#endif