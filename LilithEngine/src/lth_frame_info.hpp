#ifndef __LTH_FRAME_INFO_HPP__
#define __LTH_FRAME_INFO_HPP__

#include "lth_camera.hpp"
#include "gameObjects/lth_game_object.hpp"

#include <vulkan/vulkan.h>

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
		VkCommandBuffer commandBuffer;
		LthCamera& camera;
		VkDescriptorSet globalDescriptorSet;
		LthGameObject::Map& gameObjects;
	};
}

#endif