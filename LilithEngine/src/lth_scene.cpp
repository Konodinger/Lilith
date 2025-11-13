#include "lth_scene.hpp"
#include <iostream>

namespace lth {

	LthScene::LthScene(LthDevice& device) : lthDevice{ device } {
		defaultTexture = LthTexture::createUniqueTextureFromFile(0, lthDevice, TEXTURESFOLDERPATH(DEFAULTTEXTURE), false);
	}

	void LthScene::clear() {
		gameObjectMap.clear();
		modelMap.clear();
		textureMap.clear();
	}

	void LthScene::createTLAS() {
		//TODO
	}

	const std::shared_ptr<LthGameObject> LthScene::createGameObject() {
		auto gameObject = std::make_shared<LthGameObject>(objId++);
		gameObjectMap.insert({ gameObject->getId(), gameObject });
		return gameObject;
	}
	
	const std::shared_ptr<LthGameObject> LthScene::createPointLight(
		float intensity,
		float radius,
		glm::vec3 color) {
		auto gameObject = LthGameObject::createPointLight(objId++, intensity, radius, color);
		gameObjectMap.insert({ gameObject->getId(), gameObject });
		return gameObject;
	}

	std::shared_ptr<LthModel> LthScene::createModelFromFile(
		const std::string& filePath) {
		auto model = LthModel::createModelFromFile(modId++, lthDevice, filePath);
		modelMap.insert({ model->getId(), model });
		return model;
	}

	const std::shared_ptr<LthTexture> LthScene::createTextureFromFile(
		const std::string& textureName,
		bool generateMipmaps,
		bool addToDescriptor) {
		auto texture = LthTexture::createTextureFromFile(texId, lthDevice, TEXTURESFOLDERPATH(textureName), generateMipmaps);
		textureMap.insert({ texture->getId(), texture });

		if (addToDescriptor) {
			if (textureDescriptorArrayCount >= TEXTUREARRAYSIZE) {
				// In the future, may try to resize the texture array and the global descriptor set.
				std::cerr << "Error: texture array max size is already reached. Can't load any more texture." << std::endl;

				/*textureArray.resize(textureArray.size() + TEXTUREARRAYSIZE);
				std::cout << "The texture array is too short. Resizing to " << textureArray.size() << "..." << std::endl;
				assert(textureArray.size() < GLOBALPOOLMAXSETS && "Error: the texture array is too big and may not be handled correctly by the descriptor pool.");*/
			}
			texture->setDescriptorId(textureDescriptorArrayCount);
			textureDescriptorArray[textureDescriptorArrayCount++] = texId;
		}
		texId++;
		return texture;
	}

	std::vector<VkDescriptorImageInfo> LthScene::getDescriptorImagesInfos() {
		std::vector<VkDescriptorImageInfo> descriptorImagesInfos{};
		for (int i = 0; i < TEXTUREARRAYSIZE; ++i) {
			descriptorImagesInfos.push_back(
				(textureDescriptorArray[i] == 0)
				? defaultTexture->imageInfo()
				: textureMap.at(textureDescriptorArray[i])->imageInfo());
		}
		return descriptorImagesInfos;
	}
}