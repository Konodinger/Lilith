#ifndef __POINT_LIGHT_SYSTEM_HPP__
#define __POINT_LIGHT_SYSTEM_HPP__

#include "lth_graphics_system.hpp"
#include "../gameObjects/lth_game_object.hpp"
#include "../lth_global_info.hpp"

#include <memory>
#include <vector>

namespace lth {

	class LthPointLightSystem : LthGraphicsSystem {
		inline static std::string vertexShaderSpvPath = SHADERSFOLDERPATH("pointLight.vert.spv");
		inline static std::string fragmentShaderSpvPath = SHADERSFOLDERPATH("pointLight.frag.spv");

	public:

		LthPointLightSystem(LthDevice& device, VkRenderPass renderPass, DescriptorSetLayouts& setLayouts);
		//~LthPointLightSystem();

		LthPointLightSystem(const LthPointLightSystem&) = delete;
		LthPointLightSystem& operator=(const LthPointLightSystem&) = delete;
		
		void update(FrameInfo& frameInfo, GlobalUBO& ubo);
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