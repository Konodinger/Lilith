#ifndef __LTH_GAME_OBJECT_HPP
#define __LTH_GAME_OBJECT_HPP

#include "../lth_model.hpp"
#include "../lth_texture.hpp"
#include "../components/transform.hpp"
#include "../lth_descriptors.hpp"

#include <memory>
#include <unordered_map>

namespace lth {

	struct GameObjectUBO {
		alignas(4) bool usesColorTexture = false;
		int textureId = 0;
	};
	
	struct PointLightComponent {
		float lightIntensity = 1.f;
		float lightQuadraticAttenuation = 1.f;
	};

	class LthGameObject {
	public:
		using id_t = unsigned int;
		using Map = std::unordered_map<id_t, LthGameObject>;

		static LthGameObject createGameObject() {
			static id_t currentId = 0;
			return LthGameObject{ currentId++ };
		}

		static LthGameObject createPointLight(
			float intensity = 1.f,
			float radius = 0.05f,
			glm::vec3 color = glm::vec3(1.f));


		LthGameObject(const LthGameObject&) = delete;
		LthGameObject &operator=(const LthGameObject&) = delete;
		LthGameObject(LthGameObject&&) = default;
		LthGameObject &operator=(LthGameObject&&) = default;
		
		id_t getId() const { return id; }

		void setUsesColorTexture(bool usesColorTexture);
		void setTextureId(int textureId) { ubo.textureId = textureId; }
		void createDescriptorSet(LthDevice &lthDevice,
			LthDescriptorSetLayout* gameObjectSetLayout,
			LthDescriptorPool* generalDescriptorPool);
		void updateUBO(int frameIndex);

		glm::vec3 color{};
		Transform transform{};

		//Optional, will depend on the nature of the game object.
		std::shared_ptr<LthModel> model{};
		std::vector<VkDescriptorSet> gameObjectDescriptorSets{};
		std::vector<std::unique_ptr<LthBuffer>> gameObjectUboBuffers{};
		std::unique_ptr<PointLightComponent> pointLight = nullptr;

	private:
		LthGameObject(id_t objId) : id{objId} {}

		id_t id;
		GameObjectUBO ubo{};
	};
}

#endif