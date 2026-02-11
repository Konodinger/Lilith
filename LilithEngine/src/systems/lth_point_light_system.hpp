#ifndef __POINT_LIGHT_SYSTEM_HPP__
#define __POINT_LIGHT_SYSTEM_HPP__

#include "lth_graphics_system.hpp"
#include "../gameObjects/lth_game_object.hpp"
#include "../lth_global_info.hpp"

#include <memory>
#include <vector>

namespace lth {

	class LthPointLightSystem : public LthGraphicsSystem {
		LthGraphicsPipelineFilePaths pointLightRenderFilePaths = {
			.vertexFilePath = SHADERSPIRVFOLDERPATH("pointLight.vert"),
			.fragmentFilePath = SHADERSPIRVFOLDERPATH("pointLight.frag") };

	public:

		LthPointLightSystem(LthDevice& device,
			LthShaderCompiler& shaderCompiler,
			VkRenderPass renderPass,
			DescriptorSetLayouts& setLayouts);

		LthPointLightSystem(const LthPointLightSystem&) = delete;
		LthPointLightSystem& operator=(const LthPointLightSystem&) = delete;
		
		void update(FrameInfo& frameInfo, GlobalUBO& ubo);
		void render(FrameInfo &frameInfo);
	private:
		void createPipeline(VkRenderPass renderPass);
	};
}

#endif