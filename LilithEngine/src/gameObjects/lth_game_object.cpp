#include "lth_game_object.hpp"

namespace lth {

	LthGameObject LthGameObject::createPointLight(float intensity, float radius, glm::vec3 color) {
		LthGameObject gameObj = LthGameObject::createGameObject();
		gameObj.color = color;
		gameObj.transform.setScaleX(radius);
		gameObj.pointLight = std::make_unique<PointLightComponent>();
		gameObj.pointLight->lightIntensity = intensity;
		return gameObj;
	}

}