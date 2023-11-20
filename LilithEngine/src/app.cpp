#include "app.hpp"

#include "systems/render_system.hpp"
#include "systems/point_light_system.hpp"
#include "lth_camera.hpp"
#include "lth_buffer.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

#include <cassert>
#include <stdexcept>
#include <array>
#include <iostream>

namespace lth {

    App::App() :
        cameraController{},
        viewerObject { LthGameObject::createGameObject() },
        startingTime{ std::chrono::high_resolution_clock::now() } {
        globalPool = LthDescriptorPool::Builder(lthDevice)
            .setMaxSets(LthSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, LthSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LthSwapChain::MAX_FRAMES_IN_FLIGHT)
            .build();
        
        loadTextures();
		loadGameObjects();

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
	}

	App::~App() {
        ImGui::DestroyContext();
    }

	void App::run() {

        std::vector<std::unique_ptr<LthBuffer>> uboBuffers(LthSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < uboBuffers.size(); ++i) {
            uboBuffers[i] = std::make_unique<LthBuffer>(
                lthDevice,
                sizeof(GlobalUBO),
                1,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            uboBuffers[i]->map();
        }

        auto globalSetLayout = LthDescriptorSetLayout::Builder(lthDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

        std::vector<VkDescriptorSet> globalDescriptorSets(LthSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < globalDescriptorSets.size(); ++i) {
            VkDescriptorBufferInfo bufferInfo = uboBuffers[i]->descriptorInfo();
            VkDescriptorImageInfo imageInfo = texture->imageInfo();
            LthDescriptorWriter(*globalSetLayout, *globalPool)
                .writeBuffer(0, &bufferInfo)
                .writeImage(1, &imageInfo)
                .build(globalDescriptorSets[i]);
        }

		RenderSystem renderSystem{
            lthDevice,
            lthRenderer.getSwapChainRenderPass(),
            globalSetLayout->getDescriptorSetLayout()
        };

        PointLightSystem pointLightSystem{
            lthDevice,
            lthRenderer.getSwapChainRenderPass(),
            globalSetLayout->getDescriptorSetLayout()
        };
        LthCamera camera{};
        viewerObject.transform.setTranslation({ 0.f, -0.5f, -1.f });

        currentTime = std::chrono::high_resolution_clock::now();

		while (!lthWindow.shouldClose()) {
			glfwPollEvents();

            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;

            if (update_delta_time_mode == LTH_UPDATE_DT_MODE_CONSTANT_DT_ONE_CALL) {
                update(UPDATE_DT);
            } else {

                if (update_delta_time_mode == LTH_UPDATE_DT_MODE_ADAPTIVE_DT_CAPPED || update_delta_time_mode == LTH_UPDATE_DT_MODE_CONSTANT_DT_MULTIPLE_CALL_CAPPED) {
                    frameTime = glm::min(frameTime, MAX_FRAME_TIME);
                }

                if (update_delta_time_mode == LTH_UPDATE_DT_MODE_ADAPTIVE_DT || update_delta_time_mode == LTH_UPDATE_DT_MODE_ADAPTIVE_DT_CAPPED) {
                    update(frameTime);
                } else {
                    frameTimeAccumulator += frameTime;

                    while (frameTimeAccumulator >= UPDATE_DT_HALF) {
                        update(UPDATE_DT);
                        frameTimeAccumulator -= UPDATE_DT;
                    }
                }
            }

            std::cout << viewerObject.transform.getTranslation().x << " " << viewerObject.transform.getTranslation().y << " " << viewerObject.transform.getTranslation().z << std::endl;
            camera.setViewQuat(viewerObject.transform.getTranslation(), viewerObject.transform.getRotationMatrix());
            float aspect = lthRenderer.getAspectRatio(); // Might change when the window is resized.
            camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 1000.f);

			if (auto commandBuffer = lthRenderer.beginFrame()) {
                int frameIndex = lthRenderer.getFrameIndex();
                FrameInfo frameInfo{
                    frameIndex,
                    frameTime,
                    commandBuffer,
                    camera,
                    globalDescriptorSets[frameIndex],
                    gameObjects
                };

                // Update uniform buffers.
                GlobalUBO ubo{};
                ubo.projectionMatrix = camera.getProjection();
                ubo.viewMatrix = camera.getView();
                ubo.inverseViewMatrix = camera.getInverseView();
                pointLightSystem.update(frameInfo, ubo);
                uboBuffers[frameIndex]->writeToBuffer(&ubo);
                uboBuffers[frameIndex]->flush();

                // Render the scene.
				lthRenderer.beginSwapChainRenderPass(commandBuffer);
				renderSystem.renderGameObjects(frameInfo);
                pointLightSystem.render(frameInfo);
				lthRenderer.endSwapChainRenderPass(commandBuffer);
				lthRenderer.endFrame();
			}
		}

		vkDeviceWaitIdle(lthDevice.device());
	}

    // Update the component of the scene according to the different inputs.
    void App::update(float dt) {

        cameraController.moveInPlaneXZ(lthWindow.getGLFWwindow(), dt, viewerObject);
    }

	void App::loadGameObjects() {
		
        std::shared_ptr<LthModel> lthModel = LthModel::createModelFromFile(lthDevice, MODELSFOLDERPATH("smooth_vase.obj"));

        auto smooth_vase = LthGameObject::createGameObject();
        smooth_vase.model = lthModel;
        smooth_vase.transform.setTranslation({ 0.5f, -0.5f, 0.f });
        smooth_vase.transform.setScale({ 2.f, 2.f, 2.f });

        gameObjects.emplace(smooth_vase.getId(), std::move(smooth_vase));

        lthModel = LthModel::createModelFromFile(lthDevice, MODELSFOLDERPATH("flat_vase.obj"));

        auto flat_vase = LthGameObject::createGameObject();
        flat_vase.model = lthModel;
        flat_vase.transform.setTranslation({ -0.5f, -0.5f, 0.f });
        flat_vase.transform.setScale({ 2.f, 2.f, 2.f });

        gameObjects.emplace(flat_vase.getId(), std::move(flat_vase));

        lthModel = LthModel::createModelFromFile(lthDevice, MODELSFOLDERPATH("quad.obj"));

        auto floor = LthGameObject::createGameObject();
        floor.model = lthModel;
        floor.transform.setTranslation({ 0.f, 0.f, 0.f });
        floor.transform.setScale({ 3.f, 3.f, 3.f});

        gameObjects.emplace(floor.getId(), std::move(floor));

        lthModel = LthModel::createModelFromFile(lthDevice, MODELSFOLDERPATH("viking_room.obj"));

        auto viking_room = LthGameObject::createGameObject();
        viking_room.model = lthModel;
        viking_room.transform.setTranslation({ 0.f, 0.f, 5.f });
        viking_room.transform.setScale({ 1.f, 1.f, 1.f });

        gameObjects.emplace(viking_room.getId(), std::move(viking_room));

        //Point lights
        std::vector<glm::vec3> lightColors{
          {1.f, .1f, .1f},
          {.1f, .1f, 1.f},
          {.1f, 1.f, .1f},
          {1.f, 1.f, .1f},
          {.1f, 1.f, 1.f},
          {1.f, 1.f, 1.f}
        };


        for (int i = 0; i < lightColors.size(); i++) {
            auto pointLight = LthGameObject::createPointLight(1.f);
            pointLight.color = lightColors[i];
            auto rotateLight = glm::rotate(
                glm::mat4(1.f),
                (i * glm::two_pi<float>()) / lightColors.size(),
                { 0.f, -1.f, 0.f });
            pointLight.transform.setTranslation(glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f)));
            gameObjects.emplace(pointLight.getId(), std::move(pointLight));
        }
	}

    void App::loadTextures() {
        texture = LthTexture::createTextureFromFile(lthDevice, TEXTURESFOLDERPATH("viking_room.png"), true);
    }
}