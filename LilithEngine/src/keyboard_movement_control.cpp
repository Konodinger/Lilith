#include "keyboard_movement_control.hpp"

#include <limits>
#include <iostream>
#include "lth_utils.hpp"

namespace lth {

	void KeyboardMovementController::moveInPlaneXZ(
		GLFWwindow* window,
		float dt, LthGameObject& gameObject) {
		glm::vec3 rotate{ 0 };
		if (glfwGetKey(window, keys.lookRight) == GLFW_PRESS) rotate.x -= 1.f;
		if (glfwGetKey(window, keys.lookLeft) == GLFW_PRESS) rotate.x += 1.f;
		if (glfwGetKey(window, keys.lookUp) == GLFW_PRESS) rotate.y -= 1.f;
		if (glfwGetKey(window, keys.lookDown) == GLFW_PRESS) rotate.y += 1.f;

		if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) {
			glm::vec3 rotation = gameObject.transform.getRotationEuler() + lookSpeed * dt * glm::normalize(rotate);
			rotation.y = PI - glm::clamp(glm::mod(PI - rotation.y, TWO_PI), PI_OVER_TWO, THREE_PI_OVER_TWO);
			gameObject.transform.setRotationEuler(rotation);
		}

		const glm::vec3 forwardDir = gameObject.transform.getForward();
		const glm::vec3 rightDir{forwardDir.z, 0.f, -forwardDir.x};
		const glm::vec3 upDir{ 0.f, -1.f, 0.f };

		glm::vec3 displacement{ 0 };
		if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) displacement += forwardDir;
		if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) displacement -= forwardDir;
		if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) displacement += rightDir;
		if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) displacement -= rightDir;
		if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) displacement += upDir;
		if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) displacement -= upDir;

		if (glm::dot(displacement, displacement) > std::numeric_limits<float>::epsilon()) {
			gameObject.transform.addTranslation(moveSpeed * dt * glm::normalize(displacement));
		}
		
	}
}