#ifndef __LTH_SYSTEM_SET_HPP__
#define __LTH_SYSTEM_SET_HPP__

#include "lth_render_system.hpp"
#include "lth_point_light_system.hpp"
#include "lth_particle_system.hpp"

namespace lth {

	struct LthSystemSet {
		LthRenderSystem renderSystem;
		LthPointLightSystem pointLightSystem;
		LthParticleSystem particleSystem;

		LthSystemSet(LthDevice& device,
			VkRenderPass renderPass,
			DescriptorSetLayouts& setLayouts,
			std::vector<std::unique_ptr<LthBuffer>>& cboBuffers) :
			particleSystem(device, renderPass, setLayouts, cboBuffers),
			renderSystem(device, renderPass, setLayouts),
			pointLightSystem(device, renderPass, setLayouts){};
	};
}

#endif