#ifndef __KEYBOARD_MOVEMENT_CONTROLL_HPP__
#define __KEYBOARD_MOVEMENT_CONTROLL_HPP__

#include "gameObjects/lth_game_object.hpp"
#include "lth_window.hpp"

namespace lth {

	class KeyboardMovementController {
	public:
		KeyboardMovementController() {};

		struct KeyMappings {
			int moveLeft = GLFW_KEY_A;
			int moveRight = GLFW_KEY_D;
			int moveForward = GLFW_KEY_W;
			int moveBackward = GLFW_KEY_S;
			int moveUp = GLFW_KEY_E;
			int moveDown = GLFW_KEY_Q;
			int lookLeft = GLFW_KEY_LEFT;
			int lookRight = GLFW_KEY_RIGHT;
			int lookUp = GLFW_KEY_UP;
			int lookDown = GLFW_KEY_DOWN;
		};

		void moveInPlaneXZ(GLFWwindow* window, float dt, LthGameObject& gameObject);

		KeyMappings keys{};
		float moveSpeed = 3.f;
		float lookSpeed = 1.5f;
	};
}

#endif