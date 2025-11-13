#include "lth_model.hpp"
#include "lth_utils.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <cassert>
#include <cstring>
#include <unordered_map>
#include <iostream>

namespace std {
	template <>
	struct hash<lth::LthModel::Vertex> {
		size_t operator()(lth::LthModel::Vertex const& vertex) const {
			size_t seed = 0;
			lth::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
			return seed;
		}
	};
}

namespace lth {

	LthModel::LthModel(id_t modId, LthDevice& device, const LthModel::Builder &builder) : LthSceneElement(modId), lthDevice{ device } {
		createVertexBuffer(builder.vertices);
		createIndexBuffer(builder.indices);

		// Ray tracing part
		createPositionBuffer(builder.positions);
		createASGeometry(builder);
		createBLAS();
	}

	LthModel::~LthModel() {
		if (accStruct.handle) {
			vkDestroyAccelerationStructureKHR(lthDevice.getDevice(), accStruct.handle, nullptr);
		}
	}

	bool LthModel::Builder::loadModel(const std::string& filePath) {
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath.c_str())) {
			throw std::runtime_error("Failed to load model! " + warn + err); //Will need to become a basic loading error when we will be able to load on command.
			return false;
		}

		positions.clear();
		vertices.clear();
		indices.clear();

		std::unordered_map<Vertex, uint32_t> uniqueVertices{};

		for (const auto& shape : shapes) {
			for (const auto& index : shape.mesh.indices) {
				Vertex vertex{};

				if (index.vertex_index >= 0) {
					vertex.position = {
						attrib.vertices[3 * index.vertex_index],
						attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2],
					};

					vertex.color = {
						attrib.colors[3 * index.vertex_index],
						attrib.colors[3 * index.vertex_index + 1],
						attrib.colors[3 * index.vertex_index + 2],
					};
				}

				if (index.normal_index >= 0) {
					vertex.normal = {
						attrib.normals[3 * index.normal_index],
						attrib.normals[3 * index.normal_index + 1],
						attrib.normals[3 * index.normal_index + 2],
					};
				}
				
				if (index.texcoord_index >= 0) {
					vertex.uv = {
						attrib.texcoords[2 * index.texcoord_index],
						1.0f - attrib.texcoords[2 * index.texcoord_index + 1],
					};
				}

				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
					positions.push_back(vertex.position);
					vertices.push_back(vertex);
				}

				indices.push_back(uniqueVertices[vertex]);
			}
		}
		return true;
	}

	std::shared_ptr<LthModel> LthModel::createModelFromFile(id_t modId, LthDevice& device, const std::string& filePath) {
		Builder builder{};
		builder.loadModel(filePath);
		return std::shared_ptr<LthModel>(new LthModel(modId, device, builder));
	}

	void LthModel::createVertexBuffer(const std::vector<Vertex>& vertices) {
		vertexCount = static_cast<uint32_t>(vertices.size());
		assert(vertexCount >= 3 && "Vertex count must be at least 3");
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;
		uint32_t vertexSize = sizeof(vertices[0]);

		LthBuffer stagingBuffer{
			lthDevice,
			vertexSize,
			vertexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};

		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void*)vertices.data());

		vertexBuffer = std::make_unique<LthBuffer>(
			lthDevice,
			vertexSize,
			vertexCount,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		lthDevice.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
	}

	void LthModel::createPositionBuffer(const std::vector<glm::vec3>& positions) {
		uint32_t positionCount = static_cast<uint32_t>(positions.size());
		assert(positionCount >= 3 && "Positioncount must be at least 3");
		VkDeviceSize bufferSize = sizeof(positions[0]) * positionCount;
		uint32_t positionSize = sizeof(positions[0]);

		LthBuffer stagingBuffer{
			lthDevice,
			positionSize,
			positionCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};

		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void*)positions.data());

		positionBuffer = std::make_unique<LthBuffer>(
			lthDevice,
			positionSize,
			positionCount,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		lthDevice.copyBuffer(stagingBuffer.getBuffer(), positionBuffer->getBuffer(), bufferSize);
	}

	void LthModel::createIndexBuffer(const std::vector<uint32_t>& indices) {
		indexCount = static_cast<uint32_t>(indices.size());

		hasIndexBuffer = (indexCount > 0);

		if (!hasIndexBuffer) return;

		uint32_t indexSize = sizeof(indices[0]);

		LthBuffer stagingBuffer{
			lthDevice,
			indexSize,
			indexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};

		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void*)indices.data());
		stagingBuffer.unmap();

		indexBuffer = std::make_unique<LthBuffer>(
			lthDevice,
			indexSize,
			indexCount,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
			| VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		lthDevice.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), stagingBuffer.getBufferSize());
	}

	void LthModel::createASGeometry(const Builder& builder) {
		if (!positionBuffer || !indexBuffer) {
			throw std::runtime_error("AS geometry cannot be created if the model buffers are not set correctly.");
		}

		VkBufferDeviceAddressInfo positionBufferDeviceAddressInfo{
			.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
			.pNext = nullptr,
			.buffer = positionBuffer->getBuffer(),
		};

		auto positionBufferDeviceAddress = vkGetBufferDeviceAddress(lthDevice.getDevice(), &positionBufferDeviceAddressInfo);

		VkBufferDeviceAddressInfo indexBufferDeviceAddressInfo{
			.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
			.pNext = nullptr,
			.buffer = indexBuffer->getBuffer(),
		};

		auto indexBufferDeviceAddress = vkGetBufferDeviceAddress(lthDevice.getDevice(), &indexBufferDeviceAddressInfo);

		VkAccelerationStructureGeometryTrianglesDataKHR triangles{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
		.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
		.vertexData = {.deviceAddress = positionBufferDeviceAddress},
		.vertexStride = sizeof(glm::vec3),
		.maxVertex = vertexCount - 1,
		.indexType = VK_INDEX_TYPE_UINT32,
		.indexData = {.deviceAddress = indexBufferDeviceAddress},
		};

		asGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		asGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		asGeometry.geometry = { .triangles = triangles };
		asGeometry.flags = VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR | VK_GEOMETRY_OPAQUE_BIT_KHR;

		asBuildRangeInfo.primitiveCount = indexCount / 3;
		asBuildRangeInfo.primitiveOffset = 0;
		asBuildRangeInfo.firstVertex = 0;
		asBuildRangeInfo.transformOffset = 0;
	}

	void LthModel::createBLAS() {
		VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
		accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		accelerationStructureBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationStructureBuildGeometryInfo.geometryCount = 1;
		accelerationStructureBuildGeometryInfo.pGeometries = &asGeometry;

		VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
		accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

		vkGetAccelerationStructureBuildSizesKHR(
			lthDevice.getDevice(),
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&accelerationStructureBuildGeometryInfo,
			&asBuildRangeInfo.primitiveCount,
			&accelerationStructureBuildSizesInfo);

		uint32_t scratchOffsetAlignment = lthDevice.accelStructProperties.minAccelerationStructureScratchOffsetAlignment;
		VkDeviceSize scratchSize = ((accelerationStructureBuildSizesInfo.buildScratchSize + scratchOffsetAlignment - 1) & ~(scratchOffsetAlignment - 1));

		LthBuffer scratchBuffer{
			lthDevice,
			scratchSize,
			1,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			scratchOffsetAlignment
		};

		VkBufferDeviceAddressInfo scratchBufferDeviceAddressInfo{
			.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
			.pNext = nullptr,
			.buffer = scratchBuffer.getBuffer(),
		};

		auto scratchBufferDeviceAddress = vkGetBufferDeviceAddress(lthDevice.getDevice(), &scratchBufferDeviceAddressInfo);

		accStruct.buffer = std::make_unique<LthBuffer>(
			lthDevice,
			accelerationStructureBuildSizesInfo.accelerationStructureSize,
			1,
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
		accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		accelerationStructureCreateInfo.buffer = accStruct.buffer->getBuffer();
		accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
		accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

		vkCreateAccelerationStructureKHR(lthDevice.getDevice(), &accelerationStructureCreateInfo, nullptr, &accStruct.handle);

		accelerationStructureBuildGeometryInfo.dstAccelerationStructure = accStruct.handle;
		accelerationStructureBuildGeometryInfo.scratchData.deviceAddress = scratchBufferDeviceAddress;

		lthDevice.buildAccelerationStructure(accelerationStructureBuildGeometryInfo, asBuildRangeInfo);

		VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
		accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		accelerationDeviceAddressInfo.accelerationStructure = accStruct.handle;

		accStruct.deviceAddress =
			vkGetAccelerationStructureDeviceAddressKHR(lthDevice.getDevice(), &accelerationDeviceAddressInfo);
	}

	void LthModel::draw(VkCommandBuffer commandBuffer) {
		if (hasIndexBuffer) {
			vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
		}
		else {
			vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
		}
	}

	void LthModel::bind(VkCommandBuffer commandBuffer) {
		VkBuffer buffers[] = { vertexBuffer->getBuffer()};
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

		if (hasIndexBuffer) {
			vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
		}
	}


	std::vector<VkVertexInputBindingDescription> LthModel::Vertex::getBindingDescriptions() {
		std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(Vertex);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescriptions;
	}

	std::vector<VkVertexInputAttributeDescription> LthModel::Vertex::getAttributeDescriptions() {
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

		attributeDescriptions.push_back({ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) });
		attributeDescriptions.push_back({ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) });
		attributeDescriptions.push_back({ 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) });
		attributeDescriptions.push_back({ 3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv) });

		return attributeDescriptions;
	}
}