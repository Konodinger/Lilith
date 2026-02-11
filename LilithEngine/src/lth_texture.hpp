#ifndef __LTH_TEXTURE_HPP__
#define __LTH_TEXTURE_HPP__

#include "lth_device.hpp"
#include "lth_buffer.hpp"
#include "lth_scene_element.hpp"
#include "lth_renderer.hpp"

#include <stb_image.h>

#include <memory>

namespace lth {

	class LthTexture : public LthSceneElement {
	public:
		struct Builder {
			stbi_uc* pixels = nullptr;
			int texWidth, texHeight, texChannels;
			VkDeviceSize pixelSize;
			VkFormat texFormat;
			VkDeviceSize texSize() const { return pixelSize * texWidth * texHeight; }
			bool loadTexture(const std::string& filepath);
		};

		LthTexture(id_t texId, LthDevice& device, const LthRenderer& renderer);
		LthTexture(id_t texId, LthDevice& device, const LthTexture::Builder& builder, bool generateMipmaps);
		~LthTexture();
		
		static std::shared_ptr<LthTexture> createTextureFromFile(id_t texId, LthDevice& device, const std::string& filePath, bool generateMipmaps = true);
		static std::unique_ptr<LthTexture> createUniqueTextureFromFile(id_t texId, LthDevice& device, const std::string& filePath, bool generateMipmaps = true);

		LthTexture(const LthTexture&) = delete;
		LthTexture& operator=(const LthTexture&) = delete;
		LthTexture(LthTexture&&) = default;
		LthTexture& operator=(LthTexture&&) = default;

		VkImage getImage() { return textureImage; }
		uint32_t width() { return texWidth; }
		uint32_t height() { return texHeight; }
		VkFormat getFormat() { return texFormat; }
		VkImageLayout getLayout() { return currentImageLayout; }
		uint32_t getMipLevels() { return mipLevels; }

		void transitionImageLayout(VkImageLayout imageLayout);
		void clearImage(VkClearColorValue* clearColor);
		void resizeImage(const VkExtent2D& extent);

		uint32_t getDescriptorId() const { return textureDescriptorId; }
		void setDescriptorId(uint32_t descId) { textureDescriptorId = descId; }

		VkDescriptorImageInfo imageInfo();

		VkImageView textureImageView;
		VkSampler textureSampler;
	private:
		void createTextureImage(stbi_uc* pixels, VkDeviceSize pixelSize);
		void createTextureImageView();
		void createTextureSampler();
		void generateMipmaps(VkImageLayout newImageLayout); // Indicate the output layout.

		LthDevice& lthDevice;
		VkImage textureImage;
		VkImageLayout currentImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkDeviceMemory textureImageMemory;

		uint32_t textureDescriptorId = -1;
		uint32_t texWidth, texHeight;
		VkFormat texFormat;
		uint32_t mipLevels;
	};
}

#endif