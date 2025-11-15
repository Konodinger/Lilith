#include "lth_scene.hpp"
#include <iostream>

namespace lth {

	LthScene::LthScene(LthDevice& device) : lthDevice{ device } {
		defaultTexture = LthTexture::createUniqueTextureFromFile(0, lthDevice, TEXTURESFOLDERPATH(DEFAULTTEXTURE), false);
	}

	void LthScene::clear() {

		if (tlas.handle) {
			vkDestroyAccelerationStructureKHR(lthDevice.getDevice(), tlas.handle, nullptr);
		}

		gameObjectMap.clear();
		modelMap.clear();
		textureMap.clear();
	}

	void LthScene::createTLAS() {

		auto toTransformMatrixKHR = [](const glm::mat4& m) {
			VkTransformMatrixKHR t;
			memcpy(&t, glm::value_ptr(glm::transpose(m)), sizeof(t));
			return t;
			};
		
		std::vector<VkAccelerationStructureInstanceKHR> tlasInstances;
		tlasInstances.reserve(instanceArray.size());
		for (auto& instance : instanceArray)
		{
			VkAccelerationStructureInstanceKHR asInstance{};
			asInstance.transform = toTransformMatrixKHR(gameObjectMap.at(instance.first)->transform.modelMatrix());
			asInstance.instanceCustomIndex = instance.second; // Currently unused
			asInstance.accelerationStructureReference = modelMap.at(instance.second)->getBLASAddress();
			asInstance.instanceShaderBindingTableRecordOffset = 0;
			asInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
			asInstance.mask = 0xFF;
			tlasInstances.emplace_back(asInstance);
		}

		auto instancesBuffer = std::make_unique<LthBuffer>(
			lthDevice,
			sizeof(VkAccelerationStructureInstanceKHR),
			tlasInstances.size(),
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);
		instancesBuffer->map();
		instancesBuffer->writeToBuffer(tlasInstances.data(), sizeof(VkAccelerationStructureInstanceKHR) * tlasInstances.size());
		
		VkBufferDeviceAddressInfo tlasInstanceBufferDeviceAddressInfo{
			.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
			.pNext = nullptr,
			.buffer = instancesBuffer->getBuffer(),
		};
		auto instancesBufferDeviceAddress = vkGetBufferDeviceAddress(lthDevice.getDevice(), &tlasInstanceBufferDeviceAddressInfo);

		VkAccelerationStructureGeometryKHR asGeometry{};
		asGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		asGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		asGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		asGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		asGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
		asGeometry.geometry.instances.data.deviceAddress = instancesBufferDeviceAddress;

		VkAccelerationStructureBuildRangeInfoKHR asBuildRangeInfo{};
		asBuildRangeInfo.primitiveCount = tlasInstances.size();
		asBuildRangeInfo.primitiveOffset = 0;
		asBuildRangeInfo.firstVertex = 0;
		asBuildRangeInfo.transformOffset = 0;

		// (Almost) common part with blas
		buildAccelerationStructure(lthDevice, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, asGeometry, asBuildRangeInfo, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR, tlas);
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

	std::vector<VkDescriptorImageInfo> const LthScene::getDescriptorImagesInfos() {
		std::vector<VkDescriptorImageInfo> descriptorImagesInfos{};
		for (int i = 0; i < TEXTUREARRAYSIZE; ++i) {
			descriptorImagesInfos.push_back(
				(textureDescriptorArray[i] == 0)
				? defaultTexture->imageInfo()
				: textureMap.at(textureDescriptorArray[i])->imageInfo());
		}
		return descriptorImagesInfos;
	}

	void LthScene::linkGameObjectToModel(id_t gameObjectId, id_t modelId) {
		if (gameObjectMap.at(gameObjectId)->getModelId() != modelId) {
			gameObjectMap.at(gameObjectId)->setModel(modelId);
			instanceArray.insert_or_assign(gameObjectId, modelId);
		}
	}

	void LthScene::unlinkGameObjectToModel(id_t gameObjectId) {
		gameObjectMap.at(gameObjectId)->setModel(0);
		instanceArray.erase(gameObjectId);
	}
}