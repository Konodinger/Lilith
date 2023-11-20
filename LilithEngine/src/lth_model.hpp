#ifndef __LTH_MODEL_HPP__
#define __LTH_MODEL_HPP__

#include "lth_device.hpp"
#include "lth_buffer.hpp"

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
		void createVertexBuffers(const std::vector<Vertex>& vertices);
		void createIndexBuffers(const std::vector<uint32_t>& indices);

		LthDevice& lthDevice;
		
		std::unique_ptr<LthBuffer> vertexBuffer;
		uint32_t vertexCount;

		bool hasIndexBuffer = false;
		std::unique_ptr<LthBuffer> indexBuffer;
		uint32_t indexCount;
	};
}

#endif