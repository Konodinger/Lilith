#ifndef __LTH_SCENE_HPP__
#define __LTH_SCENE_HPP__

#include "lth_scene_element.hpp"
#include "lth_model.hpp"
#include "lth_texture.hpp"
#include "gameObjects/lth_game_object.hpp"
#include "lth_global_info.hpp"

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
		
		const std::shared_ptr<LthTexture> createTextureFromFile(
			const std::string& textureName,
			bool generateMipmaps = true,
			bool addToDescriptor = true);
		inline const std::shared_ptr<LthTexture> texture(id_t index) const { return textureMap.at(index); }
		inline const ElementMap<LthTexture>& textures() const { return textureMap; }

		std::vector<VkDescriptorImageInfo> getDescriptorImagesInfos();


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
	};
}

#endif