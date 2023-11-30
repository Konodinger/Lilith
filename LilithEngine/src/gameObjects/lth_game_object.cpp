#include "lth_game_object.hpp"

#include "../lth_swap_chain.hpp"
#include <iostream>

namespace lth {

	LthGameObject LthGameObject::createPointLight(float intensity, float radius, glm::vec3 color) {
		LthGameObject gameObj = LthGameObject::createGameObject();
		gameObj.color = color;
		gameObj.transform.setScaleX(radius);
		gameObj.pointLight = std::make_unique<PointLightComponent>();
		gameObj.pointLight->lightIntensity = intensity;
		return gameObj;
	}

	void LthGameObject::setUsesColorTexture(bool usesColorTexture) {
		ubo.usesColorTexture = usesColorTexture;
	}

	void LthGameObject::createDescriptorSet(LthDevice &lthDevice,
        LthDescriptorSetLayout* gameObjectSetLayout,
        LthDescriptorPool* generalDescriptorPool) {
        gameObjectUboBuffers.resize(LthSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < gameObjectUboBuffers.size(); ++i) {
            gameObjectUboBuffers[i] = std::make_unique<LthBuffer>(
                lthDevice,
                sizeof(GameObjectUBO),
                1,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            gameObjectUboBuffers[i]->map();
        }

        gameObjectDescriptorSets.resize(LthSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < gameObjectDescriptorSets.size(); ++i) {
            VkDescriptorBufferInfo bufferInfo = gameObjectUboBuffers[i]->descriptorInfo();
            LthDescriptorWriter(*gameObjectSetLayout, *generalDescriptorPool)
                .writeBuffer(0, &bufferInfo)
                .build(gameObjectDescriptorSets[i]);
        }
	}

    void LthGameObject::updateUBO(int frameIndex) {
        
        //
        // No update for now.
        //

        gameObjectUboBuffers[frameIndex]->writeToBuffer(&ubo);
        gameObjectUboBuffers[frameIndex]->flush();
    }
}