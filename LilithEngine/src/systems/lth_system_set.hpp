#ifndef __LTH_SYSTEM_SET_HPP__
#define __LTH_SYSTEM_SET_HPP__

#include "lth_render_system.hpp"
#include "lth_point_light_system.hpp"
#include "lth_particle_system.hpp"
#include "lth_ray_tracing_system.hpp"

namespace lth {

	struct LthSystemSet {
		LthRenderSystem renderSystem;
		LthRayTracingSystem rayTracingSystem;
		LthPointLightSystem pointLightSystem;
		LthParticleSystem particleSystem;

		LthSystemSet(LthDevice& device,
			LthShaderCompiler& shaderCompiler,
			VkRenderPass renderPass,
			DescriptorSetLayouts& setLayouts,
			std::vector<std::unique_ptr<LthBuffer>>& cboBuffers) :
			particleSystem(device, shaderCompiler, renderPass, setLayouts, cboBuffers),
			renderSystem(device, shaderCompiler, renderPass, setLayouts),
			rayTracingSystem(device, shaderCompiler, renderPass, setLayouts),
			pointLightSystem(device, shaderCompiler, renderPass, setLayouts){};
	};
}

#endif