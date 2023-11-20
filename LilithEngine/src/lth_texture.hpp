#ifndef __LTH_TEXTURE_HPP__
#define __LTH_TEXTURE_HPP__

#include "lth_device.hpp"
#include "lth_buffer.hpp"

#include <stb_image.h>

#include <memory>

namespace lth {

	class LthTexture {
	public:
		struct Builder {
			stbi_uc* pixels = nullptr;
			int texWidth, texHeight, texChannels;
			VkDeviceSize pixelSize;
			VkFormat texFormat;
			VkDeviceSize texSize() const { return pixelSize * texWidth * texHeight; }
			bool loadTexture(const std::string& filepath);
		};

		LthTexture(LthDevice& device, const LthTexture::Builder& builder, bool generateMipmaps);
		~LthTexture();

		LthTexture(const LthTexture&) = delete;
		LthTexture& operator=(const LthTexture&) = delete;

		VkImage getImage() { return textureImage; }
		uint32_t width() { return texWidth; }
		uint32_t height() { return texHeight; }
		VkFormat getFormat() { return texFormat; }
		uint32_t getMipLevels() { return mipLevels; }

		static std::unique_ptr<LthTexture> createTextureFromFile(LthDevice& device, const std::string& filePath, bool generateMipmaps = true);
		VkDescriptorImageInfo imageInfo();

		VkImageView textureImageView;
		VkSampler textureSampler;
	private:
		void createTextureImage(stbi_uc* pixels);
		void createTextureImageView();
		void createTextureSampler();

		LthDevice& lthDevice;
		VkImage textureImage;
		VkDeviceMemory textureImageMemory;

		uint32_t texWidth, texHeight;
		VkDeviceSize pixelSize;
		VkFormat texFormat;
		uint32_t mipLevels;
	};
}

#endif