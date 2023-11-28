#ifndef __APP_HPP__
#define __APP_HPP__

#include "lth_window.hpp"
#include "lth_device.hpp"
#include "lth_renderer.hpp"
#include "lth_descriptors.hpp"
#include "lth_texture.hpp"
#include "keyboard_movement_control.hpp"
#include "gameObjects/lth_game_object.hpp"

#include "imgui.h"

#include <memory>
#include <vector>
#include <chrono>

#define MODELSFOLDERPATH(fileName) "models/" fileName
#define TEXTURESFOLDERPATH(fileName) "textures/" fileName
#define IMGUIFONTSFOLDERPATH(fileName) "src/libraries/imgui/misc/fonts/" fileName

namespace lth {

	class App {
	public:

		enum {
			LTH_UPDATE_DT_MODE_CONSTANT_DT_ONE_CALL,
			LTH_UPDATE_DT_MODE_CONSTANT_DT_MULTIPLE_CALL,
			LTH_UPDATE_DT_MODE_CONSTANT_DT_MULTIPLE_CALL_CAPPED,
			LTH_UPDATE_DT_MODE_ADAPTIVE_DT,
			LTH_UPDATE_DT_MODE_ADAPTIVE_DT_CAPPED
		} update_delta_time_mode = LTH_UPDATE_DT_MODE_CONSTANT_DT_MULTIPLE_CALL_CAPPED;

		static constexpr int WIDTH = 1200;
		static constexpr int HEIGHT = 800;

		static constexpr uint32_t GLOBALPOOLMAXSETS = 100;

		bool LTH_CONSTANT_UPDATE_DT = true; //This parameters is in caps lock because it could become an enum in the future.
		static constexpr float MAX_FRAME_TIME = 0.25f;
		static constexpr float UPDATE_DT = 0.01f;
		static constexpr float UPDATE_DT_HALF = UPDATE_DT / 2.f;

		App();
		~App();

		App(const App&) = delete;
		App& operator=(const App&) = delete;

		void run();
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
		std::vector<VkDescriptorSet> globalDescriptorSets{};

		LthGameObject::Map gameObjects;
		std::shared_ptr<LthTexture> texture;

		//TODO : add a camera game object;
		KeyboardMovementController cameraController;
		LthGameObject viewerObject;

		const std::chrono::steady_clock::time_point startingTime;
		std::chrono::steady_clock::time_point currentTime{};
		float frameTimeAccumulator = 0;
	};
}

#endif