#include "lth_texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <stdexcept>

namespace lth {

	LthTexture::LthTexture(LthDevice& device, const LthTexture::Builder& builder, bool generateMipMaps) :
		lthDevice{ device }, texWidth{ static_cast<uint32_t>(builder.texWidth) }, texHeight{ static_cast<uint32_t>(builder.texHeight) },
		pixelSize{ builder.pixelSize }, texFormat{ builder.texFormat } {
		

		mipLevels = generateMipMaps ?
			static_cast<uint32_t>(floor(log2(std::max(texWidth, texHeight)))) + 1 :
			1;
		createTextureImage(builder.pixels);
		createTextureImageView();
		createTextureSampler();
	}

	LthTexture::~LthTexture() {
		vkDestroySampler(lthDevice.device(), textureSampler, nullptr);
		vkDestroyImageView(lthDevice.device(), textureImageView, nullptr);
		vkDestroyImage(lthDevice.device(), textureImage, nullptr);
		vkFreeMemory(lthDevice.device(), textureImageMemory, nullptr);
	};

	bool LthTexture::Builder::loadTexture(const std::string& file) {
        pixels = stbi_load(file.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        if (!pixels) {
            throw std::runtime_error("Failed to load texture image!"); // Will need to become a basic loading error when we will be able to load on command.
			return false;
		}

        texFormat = VK_FORMAT_R8G8B8A8_SRGB; // Will be passed as an argument in the future.
        pixelSize = 4; // Depends on the format.
		return true;
	}

	std::unique_ptr<LthTexture> LthTexture::createTextureFromFile(LthDevice& device, const std::string& filePath, bool generateMipmaps) {
		Builder builder{};
		builder.loadTexture(filePath);

		return std::make_unique<LthTexture>(device, builder, generateMipmaps);
	}

    void LthTexture::createTextureImage(stbi_uc* pixels) {

		uint32_t pixelCount = texWidth * texHeight;
		LthBuffer stagingBuffer{
			lthDevice,
			pixelSize,
			pixelCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};

		stagingBuffer.map();
		stagingBuffer.writeToBuffer(pixels);

		stbi_image_free(pixels);

		lthDevice.createImage(
			texWidth,
			texHeight,
			texFormat,
			VK_IMAGE_TILING_OPTIMAL,
			(VK_IMAGE_USAGE_TRANSFER_SRC_BIT * (mipLevels > 1)) | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			textureImage,
			textureImageMemory,
			mipLevels,
			VK_SAMPLE_COUNT_1_BIT);

		lthDevice.transitionImageLayout(textureImage,
			texFormat,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			mipLevels);
		lthDevice.copyBufferToImage(stagingBuffer.getBuffer(),
			textureImage,
			texWidth,
			texHeight,
			1);
		if (mipLevels == 1) {
			lthDevice.transitionImageLayout(textureImage,
				texFormat,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				mipLevels);
		} else {
			lthDevice.generateMipmaps(textureImage, texFormat, texWidth, texHeight, mipLevels);
		}
    }

	void LthTexture::createTextureImageView() {
		textureImageView = lthDevice.createImageView(textureImage, texFormat, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
	}

	void LthTexture::createTextureSampler() {
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = lthDevice.physicalDeviceProperties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.f;
		samplerInfo.minLod = 0.f;
		samplerInfo.maxLod = static_cast<float>(mipLevels);

		if (vkCreateSampler(lthDevice.device(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create texture sampler!");
		}

	}

	VkDescriptorImageInfo LthTexture::imageInfo() {
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = textureImageView;
		imageInfo.sampler = textureSampler;

		return imageInfo;
	}
}