#ifndef __APP_HPP__
#define __APP_HPP__

#include "lth_global_info.hpp"
#include "lth_window.hpp"
#include "lth_device.hpp"
#include "lth_renderer.hpp"
#include "lth_descriptors.hpp"
#include "lth_texture.hpp"
#include "keyboard_movement_control.hpp"
#include "gameObjects/lth_game_object.hpp"

#include <memory>
#include <vector>
#include <chrono>

namespace lth {

	class App {
	public:

		App();
		~App();

		App(const App&) = delete;
		App& operator=(const App&) = delete;

		void run();
		uint32_t loadTexture(const std::string& textureName);
		inline float getAppTimer() { return std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startingTime).count(); };

	private:

		void loadGameObjects();
		void loadTextures();
		void createDescriptorSets();
		void initImGui();

		void update(float dt);

		LthWindow lthWindow{ WIDTH, HEIGHT, "Hello I'm Lilith!" };
		LthDevice lthDevice{ lthWindow };
		LthRenderer lthRenderer{ lthWindow, lthDevice };

		std::unique_ptr<LthDescriptorPool> generalDescriptorPool{};
		DescriptorSetLayouts setLayouts{};
		std::vector<std::unique_ptr<LthBuffer>> uboBuffers{};
		std::vector<std::unique_ptr<LthBuffer>> cboBuffers{};
		std::vector<VkDescriptorSet> globalDescriptorSets{};
		std::vector<VkDescriptorSet> computeDescriptorSets{};

		LthGameObject::Map gameObjects;

		std::vector<std::unique_ptr<LthTexture>> textureArray{};
		uint32_t textureArrayCount = 0;
		std::unique_ptr<LthTexture> defaultTexture;

		KeyboardMovementController cameraController;
		LthGameObject viewerObject;

		const std::chrono::steady_clock::time_point startingTime;
		std::chrono::steady_clock::time_point currentTime{};
		float frameTimeAccumulator = 0;
	};
}

#endif