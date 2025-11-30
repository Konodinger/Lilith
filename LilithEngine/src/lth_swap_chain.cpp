#include "lth_swap_chain.hpp"
#include "lth_texture.hpp"

// std
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

namespace lth {

    LthSwapChain::LthSwapChain(LthDevice &deviceRef, VkExtent2D windowExtent)
        : lthDevice{ deviceRef }, windowExtent{ windowExtent } {
        init();
    }
    LthSwapChain::LthSwapChain(LthDevice& deviceRef, VkExtent2D windowExtent, std::shared_ptr<LthSwapChain> previous)
        : lthDevice{ deviceRef }, windowExtent{ windowExtent },
        oldSwapChain{ previous } {
        init();

        // Clean up old swap chain since it's no longer needed
        oldSwapChain = nullptr;
    }

    void LthSwapChain::init() {
      createSwapChain();
      createImageViews();
      createRenderPass();
      createColorResources();
      createDepthResources();
      createFramebuffers();
      createSyncObjects();
    }

    LthSwapChain::~LthSwapChain() {
      for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(lthDevice.getDevice(), imageView, nullptr);
      }
      swapChainImageViews.clear();

      if (swapChain != nullptr) {
        vkDestroySwapchainKHR(lthDevice.getDevice(), swapChain, nullptr);
        swapChain = nullptr;
      }

      for (int i = 0; i < colorImages.size(); i++) {
          vkDestroyImageView(lthDevice.getDevice(), colorImageViews[i], nullptr);
          vkDestroyImage(lthDevice.getDevice(), colorImages[i], nullptr);
          vkFreeMemory(lthDevice.getDevice(), colorImageMemories[i], nullptr);
      }

      for (int i = 0; i < depthImages.size(); i++) {
        vkDestroyImageView(lthDevice.getDevice(), depthImageViews[i], nullptr);
        vkDestroyImage(lthDevice.getDevice(), depthImages[i], nullptr);
        vkFreeMemory(lthDevice.getDevice(), depthImageMemories[i], nullptr);
      }

      for (auto framebuffer : mainFramebuffers) {
        vkDestroyFramebuffer(lthDevice.getDevice(), framebuffer, nullptr);
      }

      for (auto framebuffer : guiFramebuffers) {
        vkDestroyFramebuffer(lthDevice.getDevice(), framebuffer, nullptr);
      }

      vkDestroyRenderPass(lthDevice.getDevice(), mainRenderPass, nullptr);
      vkDestroyRenderPass(lthDevice.getDevice(), guiRenderPass, nullptr);

      // cleanup synchronization objects

      for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroySemaphore(lthDevice.getDevice(), imageAvailableSemaphores[i].first, nullptr);
        vkDestroyFence(lthDevice.getDevice(), computeInFlightFences[i], nullptr);
        vkDestroyFence(lthDevice.getDevice(), renderInFlightFences[i], nullptr);
      }

      for (size_t i = 0; i < imageCount(); ++i) {
        vkDestroySemaphore(lthDevice.getDevice(), computeFinishedSemaphores[i].first, nullptr);
        vkDestroySemaphore(lthDevice.getDevice(), renderFinishedSemaphores[i].first, nullptr);
      }
    }

    VkResult LthSwapChain::acquireNextImage(uint32_t *imageIndex) {
        std::vector<VkFence> waitFences = { computeInFlightFences[currentFrame], renderInFlightFences[currentFrame] };
      vkWaitForFences(
          lthDevice.getDevice(),
          static_cast<uint32_t>(waitFences.size()),
          waitFences.data(),
          VK_TRUE,
          std::numeric_limits<uint64_t>::max());

      if (imageAvailableSemaphores[currentFrame].second) {
          throw std::runtime_error("Can't acquire the next image, the semaphore is still in use.");
      }

      VkResult result = vkAcquireNextImageKHR(
          lthDevice.getDevice(),
          swapChain,
          std::numeric_limits<uint64_t>::max(),
          imageAvailableSemaphores[currentFrame].first,
          VK_NULL_HANDLE,
          imageIndex);

      imageAvailableSemaphores[currentFrame].second = true;

      return result;
    }


    void LthSwapChain::copyImageToSwapChain(VkCommandBuffer commandBuffer, LthTexture& sourceImage, uint32_t imageIndex) {
        VkImageLayout originalLayout = sourceImage.getLayout();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        // Transition image to src
        barrier.image = sourceImage.getImage();
        barrier.subresourceRange.levelCount = sourceImage.getMipLevels();
        barrier.oldLayout = originalLayout;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_NONE;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        // Transition swapChainImage to dst

        barrier.image = swapChainImages[imageIndex];
        barrier.subresourceRange.levelCount = 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        // Copy the texture into the swap chain

        VkImageCopy imageCopy{};
        imageCopy.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        imageCopy.srcOffset = { 0, 0, 0 };
        imageCopy.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        imageCopy.dstOffset = { 0, 0, 0 };
        imageCopy.extent = { swapChainExtent.width, swapChainExtent.height, 1 };
        
        vkCmdCopyImage(commandBuffer, sourceImage.getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            swapChainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);

        // Transition back swap chain's image

        barrier.image = swapChainImages[imageIndex];
        barrier.subresourceRange.levelCount = 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        // Transition back image

        barrier.image = sourceImage.getImage();
        barrier.subresourceRange.levelCount = sourceImage.getMipLevels();
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = originalLayout;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_NONE;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    }

    void LthSwapChain::submitComputeCommandBuffers(
          const VkCommandBuffer* buffer, uint32_t imageIndex) {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        vkWaitForFences(lthDevice.getDevice(), 1, &computeInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
        vkResetFences(lthDevice.getDevice(), 1, &computeInFlightFences[currentFrame]);


        std::vector<VkSemaphore> waitSemaphores{};
        if (renderFinishedSemaphores[imageIndex].second) {
            waitSemaphores.push_back(renderFinishedSemaphores[imageIndex].first);
            renderFinishedSemaphores[imageIndex].second = false;
        }
        submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
        submitInfo.pWaitSemaphores = waitSemaphores.data();

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = buffer;
        std::vector<VkSemaphore> signalSemaphores{ computeFinishedSemaphores[imageIndex].first };
        computeFinishedSemaphores[imageIndex].second = true;
        submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
        submitInfo.pSignalSemaphores = signalSemaphores.data();

        if (vkQueueSubmit(lthDevice.getComputeQueue(), 1, &submitInfo, computeInFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to submit compute command buffer!");
        };
    }

    void LthSwapChain::submitGraphicsCommandBuffers(
        const VkCommandBuffer *buffer, uint32_t imageIndex) {
      vkWaitForFences(lthDevice.getDevice(), 1, &renderInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
      vkResetFences(lthDevice.getDevice(), 1, &renderInFlightFences[currentFrame]);


      VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
      
      VkSubmitInfo submitInfo = {};
      submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submitInfo.pWaitDstStageMask = waitStages;
      submitInfo.commandBufferCount = 1;
      submitInfo.pCommandBuffers = buffer;
      
      std::vector<VkSemaphore> waitSemaphores{};
      if (computeFinishedSemaphores[imageIndex].second) {
          waitSemaphores.push_back(computeFinishedSemaphores[imageIndex].first);
          computeFinishedSemaphores[imageIndex].second = false;
      }
      if (imageAvailableSemaphores[currentFrame].second) {
          waitSemaphores.push_back(imageAvailableSemaphores[currentFrame].first);
          imageAvailableSemaphores[currentFrame].second = false;
      }
      submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
      submitInfo.pWaitSemaphores = waitSemaphores.data();

      if (renderFinishedSemaphores[imageIndex].second) {
          throw std::runtime_error("Can't submit graphics queue, the semaphore is still in use.");
      }
      std::vector<VkSemaphore> signalSemaphores { renderFinishedSemaphores[imageIndex].first };
      renderFinishedSemaphores[imageIndex].second = true;
      submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
      submitInfo.pSignalSemaphores = signalSemaphores.data();

      if (vkQueueSubmit(lthDevice.getGraphicsQueue(), 1, &submitInfo, renderInFlightFences[currentFrame]) !=
          VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer!");
      }
    }

    VkResult LthSwapChain::presentAndEndFrame(uint32_t imageIndex) {

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        std::vector<VkSemaphore> waitSemaphores{};
        if (computeFinishedSemaphores[imageIndex].second) {
            waitSemaphores.push_back(computeFinishedSemaphores[imageIndex].first);
            computeFinishedSemaphores[imageIndex].second = false;
        }
        if (renderFinishedSemaphores[imageIndex].second) {
            waitSemaphores.push_back(renderFinishedSemaphores[imageIndex].first);
            renderFinishedSemaphores[imageIndex].second = false;
        }
        presentInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
        presentInfo.pWaitSemaphores = waitSemaphores.data();

        VkSwapchainKHR swapChains[] = { swapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = &imageIndex;

        auto result = vkQueuePresentKHR(lthDevice.getPresentQueue(), &presentInfo);

        renderFinishedSemaphores[imageIndex].second = false;
        computeFinishedSemaphores[imageIndex].second = false;
        imageAvailableSemaphores[currentFrame].second = false;

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

        return result;
    }

    void LthSwapChain::createSwapChain() {
      SwapChainSupportDetails swapChainSupport = lthDevice.getSwapChainSupport();

      VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
      VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
      VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

      uint32_t swapChainImageCount = swapChainSupport.capabilities.minImageCount + 1;
      if (swapChainSupport.capabilities.maxImageCount > 0 &&
          swapChainImageCount > swapChainSupport.capabilities.maxImageCount) {
          swapChainImageCount = swapChainSupport.capabilities.maxImageCount;
      }

      VkSwapchainCreateInfoKHR createInfo = {};
      createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
      createInfo.surface = lthDevice.getSurface();

      createInfo.minImageCount = swapChainImageCount;
      createInfo.imageFormat = surfaceFormat.format;
      createInfo.imageColorSpace = surfaceFormat.colorSpace;
      createInfo.imageExtent = extent;
      createInfo.imageArrayLayers = 1;
      createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

      QueueFamilyIndices indices = lthDevice.findPhysicalQueueFamilies();
      uint32_t queueFamilyIndices[] = {indices.graphicsAndComputeFamily, indices.presentFamily};

      if (indices.graphicsAndComputeFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
      } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;      // Optional
        createInfo.pQueueFamilyIndices = nullptr;  // Optional
      }

      createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
      createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

      createInfo.presentMode = presentMode;
      createInfo.clipped = VK_TRUE;

      createInfo.oldSwapchain = oldSwapChain == nullptr ? VK_NULL_HANDLE : oldSwapChain->swapChain;

      if (vkCreateSwapchainKHR(lthDevice.getDevice(), &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swap chain!");
      }

      // We only specified a minimum number of images in the swap chain, so the implementation is
      // allowed to create a swap chain with more. That's why we'll first query the final number of
      // images with vkGetSwapchainImagesKHR, then resize the container and finally call it again to
      // retrieve the handles.
      vkGetSwapchainImagesKHR(lthDevice.getDevice(), swapChain, &swapChainImageCount, nullptr);
      swapChainImages.resize(swapChainImageCount);
      vkGetSwapchainImagesKHR(lthDevice.getDevice(), swapChain, &swapChainImageCount, swapChainImages.data());

      swapChainImageFormat = surfaceFormat.format;
      swapChainDepthFormat = findDepthFormat();
      swapChainExtent = extent;
    }

    void LthSwapChain::createImageViews() {
      swapChainImageViews.resize(swapChainImages.size());
      for (size_t i = 0; i < swapChainImages.size(); i++) {
          swapChainImageViews[i] = lthDevice.createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
      }
    }

    void LthSwapChain::createRenderPass() {
      VkAttachmentDescription colorAttachment{};
      colorAttachment.format = swapChainImageFormat;
      colorAttachment.samples = lthDevice.getMsaaSamples();
      colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

      VkAttachmentReference colorAttachmentRef{};
      colorAttachmentRef.attachment = 0;
      colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

      VkAttachmentDescription depthAttachment{};
      depthAttachment.format = swapChainDepthFormat;
      depthAttachment.samples = lthDevice.getMsaaSamples();
      depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

      VkAttachmentReference depthAttachmentRef{};
      depthAttachmentRef.attachment = 1;
      depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

      VkSubpassDescription subpass{};
      subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
      subpass.colorAttachmentCount = 1;
      subpass.pColorAttachments = &colorAttachmentRef;
      subpass.pDepthStencilAttachment = &depthAttachmentRef;

      VkSubpassDependency dependency{};
      dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
      dependency.srcAccessMask = 0;
      dependency.srcStageMask =
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
      dependency.dstSubpass = 0;
      dependency.dstStageMask =
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
      dependency.dstAccessMask =
          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

      std::vector<VkAttachmentDescription> attachments;

      if (lthDevice.isMsaaEnabled()) {

          colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

          VkAttachmentDescription colorAttachmentResolve{};
          colorAttachmentResolve.format = swapChainImageFormat;
          colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
          colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
          colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
          colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
          colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
          colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
          colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR is now on the ImGui render pass, which is the definitive final layout.
          //colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR is now on the ImGui render pass, which is the definitive final layout.

          VkAttachmentReference colorAttachmentResolveRef{};
          colorAttachmentResolveRef.attachment = 2;
          colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        subpass.pResolveAttachments = &colorAttachmentResolveRef;

        attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
      } else {
          attachments = { colorAttachment, depthAttachment };
      }
      VkRenderPassCreateInfo renderPassInfo = {};
      renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
      renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
      renderPassInfo.pAttachments = attachments.data();
      renderPassInfo.subpassCount = 1;
      renderPassInfo.pSubpasses = &subpass;
      renderPassInfo.dependencyCount = 1;
      renderPassInfo.pDependencies = &dependency;

      if (vkCreateRenderPass(lthDevice.getDevice(), &renderPassInfo, nullptr, &mainRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create main render pass!");
      }

      attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

      if (vkCreateRenderPass(lthDevice.getDevice(), &renderPassInfo, nullptr, &guiRenderPass) != VK_SUCCESS) {
          throw std::runtime_error("Failed to create gui render pass!");
      }
    }

    void LthSwapChain::createFramebuffers() {
        mainFramebuffers.resize(imageCount());
        guiFramebuffers.resize(imageCount());

        for (size_t i = 0; i < imageCount(); i++) {
            std::vector<VkImageView> attachments;
            if (lthDevice.isMsaaEnabled()) {
                attachments = { colorImageViews[i], depthImageViews[i], swapChainImageViews[i] };
            }
            else {
                attachments = { swapChainImageViews[i], depthImageViews[i] };
            }

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = mainRenderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(
                lthDevice.getDevice(),
                &framebufferInfo,
                nullptr,
                &mainFramebuffers[i]) != VK_SUCCESS) {
              throw std::runtime_error("Failed to create main framebuffer!");
            }

            framebufferInfo.renderPass = guiRenderPass;

            if (vkCreateFramebuffer(
                lthDevice.getDevice(),
                &framebufferInfo,
                nullptr,
                &guiFramebuffers[i]) != VK_SUCCESS) {
              throw std::runtime_error("Failed to create gui framebuffer!");
            }
        }
    }

    void LthSwapChain::createColorResources() {

        colorImages.resize(imageCount());
        colorImageMemories.resize(imageCount());
        colorImageViews.resize(imageCount());

        for (int i = 0; i < colorImages.size(); i++) {
            lthDevice.createImage(
                swapChainExtent.width,
                swapChainExtent.height,
                swapChainImageFormat,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                colorImages[i],
                colorImageMemories[i],
                1,
                lthDevice.getMsaaSamples());
            colorImageViews[i] = lthDevice.createImageView(colorImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        }
    }

    void LthSwapChain::createDepthResources() {

      depthImages.resize(imageCount());
      depthImageMemories.resize(imageCount());
      depthImageViews.resize(imageCount());

      for (int i = 0; i < depthImages.size(); i++) {
        lthDevice.createImage(
            swapChainExtent.width,
            swapChainExtent.height,
            swapChainDepthFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            depthImages[i],
            depthImageMemories[i],
            1,
            lthDevice.getMsaaSamples());

        depthImageViews[i] = lthDevice.createImageView(depthImages[i], swapChainDepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
      }

    }

    void LthSwapChain::createSyncObjects() {
      
      computeFinishedSemaphores.resize(imageCount());
      renderFinishedSemaphores.resize(imageCount());
      imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
      computeInFlightFences.resize(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);
      renderInFlightFences.resize(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);

      VkSemaphoreCreateInfo semaphoreInfo = {};
      semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

      VkFenceCreateInfo fenceInfo = {};
      fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

      for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
          if (vkCreateSemaphore(lthDevice.getDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i].first) !=
              VK_SUCCESS) {
              throw std::runtime_error("Failed to create semaphores for a frame!");
          }
          computeFinishedSemaphores[i].second = false;
          imageAvailableSemaphores[i].second = false;
          renderFinishedSemaphores[i].second = false;

          if (vkCreateFence(lthDevice.getDevice(), &fenceInfo, nullptr, &renderInFlightFences[i]) !=
              VK_SUCCESS ||
              vkCreateFence(lthDevice.getDevice(), &fenceInfo, nullptr, &computeInFlightFences[i]) != VK_SUCCESS) {
              throw std::runtime_error("Failed to create fences for a frame!");
          }
      }


      for (size_t i = 0; i < imageCount(); i++) {
          if (vkCreateSemaphore(lthDevice.getDevice(), &semaphoreInfo, nullptr, &computeFinishedSemaphores[i].first) !=
              VK_SUCCESS ||
              vkCreateSemaphore(lthDevice.getDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i].first) !=
              VK_SUCCESS) {
              throw std::runtime_error("Failed to create semaphores for a frame!");
          }
      }
    }

    VkSurfaceFormatKHR LthSwapChain::chooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR> &availableFormats) {
      for (const auto &availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
        //if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
          return availableFormat;
        }
      }

      return availableFormats[0];
    }

    VkPresentModeKHR LthSwapChain::chooseSwapPresentMode(
        const std::vector<VkPresentModeKHR> &availablePresentModes) {
      for (const auto &availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
          //std::cout << "Present mode: Mailbox" << std::endl;
          return availablePresentMode;
        }
      }

      // for (const auto &availablePresentMode : availablePresentModes) {
      //   if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
      //     std::cout << "Present mode: Immediate" << std::endl;
      //     return availablePresentMode;
      //   }
      // }

      //std::cout << "Present mode: V-Sync" << std::endl;
      return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D LthSwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
      if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
      } else {
        VkExtent2D actualExtent = windowExtent;
        actualExtent.width = std::max(
            capabilities.minImageExtent.width,
            std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(
            capabilities.minImageExtent.height,
            std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
      }
    }

    VkFormat LthSwapChain::findDepthFormat() {
      return lthDevice.findSupportedFormat(
          {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
          VK_IMAGE_TILING_OPTIMAL,
          VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

}
