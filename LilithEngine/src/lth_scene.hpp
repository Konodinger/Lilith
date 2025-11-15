#ifndef __LTH_SCENE_HPP__
#define __LTH_SCENE_HPP__

#include "lth_scene_element.hpp"
#include "lth_model.hpp"
#include "lth_texture.hpp"
#include "gameObjects/lth_game_object.hpp"
#include "lth_global_info.hpp"
#include "lth_acceleration_structure.hpp"

#include <type_traits>
#include <glm/glm.hpp>

namespace lth {

	class LthScene {
	public:
		LthScene(LthDevice& device);
		~LthScene() { clear(); };

		void clear();

		void createTLAS();

		const std::shared_ptr<LthGameObject> createGameObject();
		const std::shared_ptr<LthGameObject> createPointLight(
			float intensity = 1.f,
			float radius = 0.05f,
			glm::vec3 color = glm::vec3(1.f));
		inline const std::shared_ptr<LthGameObject> gameObject(id_t index) const { return gameObjectMap.at(index); }
		inline const ElementMap<LthGameObject>& gameObjects() const { return gameObjectMap; }
		
		std::shared_ptr<LthModel> createModelFromFile(
			const std::string& filePath);
		inline const std::shared_ptr<LthModel> model(id_t index) const { return modelMap.at(index); }
		inline const ElementMap<LthModel>& models() const { return modelMap; }
		
		void linkGameObjectToModel(id_t gameObjectId, id_t modelId);
		inline void linkGameObjectToModel(const std::shared_ptr<LthGameObject> gameObject, id_t modelId) {
			linkGameObjectToModel(gameObject->getId(), modelId);
		}
		inline void linkGameObjectToModel(id_t gameObjectId, const std::shared_ptr<LthModel> model) {
			linkGameObjectToModel(gameObjectId, model->getId());
		}
		inline void linkGameObjectToModel(const std::shared_ptr<LthGameObject> gameObject, const std::shared_ptr<LthModel> model) {
			linkGameObjectToModel(gameObject->getId(), model->getId());
		}

		void unlinkGameObjectToModel(id_t gameObjectId);
		inline void unlinkGameObjectToModel(const std::shared_ptr<LthGameObject> gameObject) {
			unlinkGameObjectToModel(gameObject->getModelId());
		}

		const std::shared_ptr<LthTexture> createTextureFromFile(
			const std::string& textureName,
			bool generateMipmaps = true,
			bool addToDescriptor = true);
		inline const std::shared_ptr<LthTexture> texture(id_t index) const { return textureMap.at(index); }
		inline const ElementMap<LthTexture>& textures() const { return textureMap; }

		std::vector<VkDescriptorImageInfo> const getDescriptorImagesInfos();
		const std::unordered_map<id_t, id_t>& const getInstanceArray() { return instanceArray; }


	private:
		LthDevice& lthDevice;

		ElementMap<LthGameObject> gameObjectMap;
		ElementMap<LthModel> modelMap;
		ElementMap<LthTexture> textureMap;

		id_t objId = 1;
		id_t modId = 1;
		id_t texId = 1;

		id_t textureDescriptorArray[TEXTUREARRAYSIZE] {};
		uint32_t textureDescriptorArrayCount = 0;
		std::unique_ptr<LthTexture> defaultTexture;
		static_assert(TEXTUREARRAYSIZE < GLOBALPOOLMAXSETS && "Error: the texture array is too big and may not be handled correctly by the descriptor pool.");
	
		std::unordered_map<id_t, id_t> instanceArray {}; // Map of { gameObjectId, modelId }
		AccelerationStructure tlas{};
	};
}

#endif