#ifndef __LTH_GAME_OBJECT_HPP
#define __LTH_GAME_OBJECT_HPP

#include "../lth_model.hpp"
#include "../lth_texture.hpp"
#include "../components/transform.hpp"
#include "../lth_descriptors.hpp"
#include "../lth_scene_element.hpp"

#include <memory>
#include <unordered_map>

namespace lth {

	struct GameObjectUBO {
		uint32_t usesColorTexture = 0;
		uint32_t textureId = 0;
	};
	
	struct PointLightComponent {
		float lightIntensity = 1.f;
		float lightQuadraticAttenuation = 1.f;
	};

	class LthGameObject : public LthSceneElement {
	public:
		LthGameObject(id_t objId) : LthSceneElement(objId) {}

		static std::shared_ptr<LthGameObject> createPointLight(
			id_t objId,
			float intensity,
			float radius,
			glm::vec3 color);

		LthGameObject(const LthGameObject&) = delete;
		LthGameObject &operator=(const LthGameObject&) = delete;
		LthGameObject(LthGameObject&&) = default;
		LthGameObject &operator=(LthGameObject&&) = default;
		
		void setUsesColorTexture(bool usesColorTexture) { ubo.usesColorTexture = static_cast<uint32_t>(usesColorTexture); }
		void setTexture(const std::shared_ptr<LthTexture> texture) { ubo.textureId = texture->getDescriptorId(); }
		void setTexture(uint32_t textureId) { ubo.textureId = textureId; }
		uint32_t getModelId() const { return modelId; }

		void createDescriptorSet(LthDevice &lthDevice,
			LthDescriptorSetLayout* gameObjectSetLayout,
			LthDescriptorPool* generalDescriptorPool);
		void updateUBO(int frameIndex);

		glm::vec3 color{};
		Transform transform{};

		std::vector<VkDescriptorSet> gameObjectDescriptorSets{};
		std::vector<std::unique_ptr<LthBuffer>> gameObjectUboBuffers{};
		std::unique_ptr<PointLightComponent> pointLight = nullptr;

	private:

		//Id set to 0 if no model
		uint32_t modelId = 0;
		GameObjectUBO ubo{};
		void setModel(uint32_t modId) { modelId = modId; }

		friend class LthScene;
	};
}

#endif