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
		LthGraphicsPipelineFilePaths renderFilePaths = {
			.vertexFilePath = SHADERSPIRVFOLDERPATH("standard.vert"),
			.fragmentFilePath = SHADERSPIRVFOLDERPATH("standard.frag") };
	public:

		LthRenderSystem(LthDevice& device, LthShaderCompiler& shaderCompiler, VkRenderPass renderPass, DescriptorSetLayouts& setLayouts);

		LthRenderSystem(const LthRenderSystem&) = delete;
		LthRenderSystem& operator=(const LthRenderSystem&) = delete;
		
		void render(FrameInfo &frameInfo);
	private:
		void createPipeline(VkRenderPass renderPass);
	};
}

#endif