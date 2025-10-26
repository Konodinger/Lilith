#ifndef __LTH_MODEL_HPP__
#define __LTH_MODEL_HPP__

#include "lth_device.hpp"
#include "lth_buffer.hpp"
#include "lth_acceleration_structure.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace lth {
	class LthModel {
	public:
		struct Vertex {
			glm::vec3 position{};
			glm::vec3 color{};
			glm::vec3 normal{};
			glm::vec2 uv{};

			static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
		
			bool operator==(const Vertex& otherVertex) const {
				return position == otherVertex.position
					&& color == otherVertex.color
					&& normal == otherVertex.normal
					&& uv == otherVertex.uv;
			}
		};

		struct Builder {
			std::vector<glm::vec3> positions{};
			std::vector<Vertex> vertices{};
			std::vector<uint32_t> indices{};

			bool loadModel(const std::string& filepath);
		};

		LthModel(LthDevice& device, const LthModel::Builder& builder);
		~LthModel();

		LthModel(const LthModel&) = delete;
		LthModel& operator=(const LthModel&) = delete;

		static std::unique_ptr<LthModel> createModelFromFile(LthDevice &device, const std::string& filePath);

		void bind(VkCommandBuffer commandBuffer);
		void draw(VkCommandBuffer commandBuffer);
	private:
		void createVertexBuffer(const std::vector<Vertex>& vertices);
		void createPositionBuffer(const std::vector<glm::vec3>& positions);
		void createIndexBuffer(const std::vector<uint32_t>& indices);
		void createASGeometry(const Builder& builder);
		void createBLAS();

		LthDevice& lthDevice;
		
		std::unique_ptr<LthBuffer> vertexBuffer;
		uint32_t vertexCount;

		std::unique_ptr<LthBuffer> positionBuffer;

		bool hasIndexBuffer = false;
		std::unique_ptr<LthBuffer> indexBuffer;
		uint32_t indexCount;

		VkAccelerationStructureGeometryKHR asGeometry;
		VkAccelerationStructureBuildRangeInfoKHR asBuildRangeInfo;
		AccelerationStructure accStruct{};
	};
}

#endif