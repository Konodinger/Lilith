#ifndef __LTH_CAMERA_HPP__
#define __LTH_CAMERA_HPP__

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext.hpp>

namespace lth {

	class LthCamera {
	public:
		void setOrthographicProjection(float left, float right, float top, float bottom, float near, float far);
		void setPerspectiveProjection(float dofy, float aspect, float near, float far);
		void setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up = glm::vec3{ 0.f, -1.f, 0.f });
		void setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up = glm::vec3{ 0.f, -1.f, 0.f });
		void setViewYXZ(glm::vec3 position, glm::vec3 rotation);
		void setViewQuat(glm::vec3 position, glm::quat rotation);
		void setViewMatrix(glm::vec3 position, glm::mat3 rotation);

		const glm::mat4 getProjection() const { return projectionMatrix; }
		const glm::mat4 getView() const { return viewMatrix; }
		const glm::mat4 getInverseView() const { return inverseViewMatrix; }
		const glm::vec3 getPosition() const { return glm::vec3(inverseViewMatrix[3]); }

	private:
		glm::mat4 projectionMatrix{ 1.f };
		glm::mat4 viewMatrix{ 1.f };
		glm::mat4 inverseViewMatrix{ 1.f };
	};
}

#endif