#ifndef __LTH_FRAME_INFO_HPP__
#define __LTH_FRAME_INFO_HPP__

#include "lth_camera.hpp"
#include "lth_scene.hpp"


#define MAX_LIGHTS 8

namespace lth {

	struct PointLight {
		glm::vec3 position{};
		float lightQuadraticAttenuation{ 1.f }; //intensity = lightColor.w / (1 + lightQuadraticAttenuation * distance^2)
		glm::vec4 color{};
	};

	struct GlobalUBO {
		glm::mat4 projectionMatrix{ 1.f };
		glm::mat4 viewMatrix{ 1.f };
		glm::mat4 inverseViewMatrix{ 1.f };
		glm::vec4 ambiantLightColor{ 1.f, 1.f, 1.f, .2f };
		PointLight pointLights[MAX_LIGHTS];
		int numLights;
	};

	struct FrameInfo {
		int frameIndex;
		float frameTime;
		VkCommandBuffer graphicsCommandBuffer;
		VkCommandBuffer computeCommandBuffer;
		LthCamera& camera;
		VkDescriptorSet globalDescriptorSet;
		LthScene& scene;
	};
}

#endif