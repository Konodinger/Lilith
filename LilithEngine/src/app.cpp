#include "app.hpp"

#include "systems/lth_system_set.hpp"
#include "lth_camera.hpp"
#include "lth_buffer.hpp"
#include "lth_utils.hpp"

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
        viewerTransform{},
        startingTime{ std::chrono::high_resolution_clock::now() } {

        assert(GLOBALPOOLMAXSETS >= MAX_FRAMES_IN_FLIGHT && "Error: globalPool default size is too small for the swap chain.");
        generalDescriptorPool = LthDescriptorPool::Builder(lthDevice)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, GLOBALPOOLMAXSETS)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, GLOBALPOOLMAXSETS)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_FRAMES_IN_FLIGHT * 3)
            .setMaxSets(GLOBALPOOLMAXSETS)
            .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
            .build();
        
        loadScene();
        initImGui();
	}

	App::~App() {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

	void App::run() {

        createDescriptorSets();

        LthSystemSet systemSet{
            lthDevice,
            lthRenderer.getSwapChainRenderPass(),
            setLayouts,
            cboBuffers
        };

        LthCamera camera{};
        viewerTransform.setTranslation({ 0.f, -0.5f, -1.f });

        currentTime = std::chrono::high_resolution_clock::now();

		while (!lthWindow.shouldClose()) {
			glfwPollEvents();

            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;

            if (UPDATE_DELTA_TIME_MODE == LTH_UPDATE_DT_MODE_CONSTANT_DT_ONE_CALL) {
                update(UPDATE_DT);
            } else {

                if (UPDATE_DELTA_TIME_MODE == LTH_UPDATE_DT_MODE_ADAPTIVE_DT_CAPPED || UPDATE_DELTA_TIME_MODE == LTH_UPDATE_DT_MODE_CONSTANT_DT_MULTIPLE_CALL_CAPPED) {
                    frameTime = glm::min(frameTime, MAX_FRAME_TIME);
                }

                if (UPDATE_DELTA_TIME_MODE == LTH_UPDATE_DT_MODE_ADAPTIVE_DT || UPDATE_DELTA_TIME_MODE == LTH_UPDATE_DT_MODE_ADAPTIVE_DT_CAPPED) {
                    update(frameTime);
                } else {
                    frameTimeAccumulator += frameTime;

                    while (frameTimeAccumulator >= UPDATE_DT_HALF) {
                        update(UPDATE_DT);
                        frameTimeAccumulator -= UPDATE_DT;
                    }
                }
            }

            camera.setViewQuat(viewerTransform.getTranslation(), viewerTransform.getRotationMatrix());
            float aspect = lthRenderer.getAspectRatio(); // Might change when the window is resized.
            camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 1000.f);


			if (lthRenderer.beginFrame()) {
                
                // Get the current frame.
                VkCommandBuffer graphicsCommandBuffer = lthRenderer.getCurrentGraphicsCommandBuffer();
                VkCommandBuffer computeCommandBuffer = lthRenderer.getCurrentComputeCommandBuffer();

                int frameIndex = lthRenderer.getFrameIndex();
                FrameInfo frameInfo{
                    frameIndex,
                    frameTime,
                    graphicsCommandBuffer,
                    computeCommandBuffer,
                    camera,
                    globalDescriptorSets[frameIndex],
                    scene
                };

                // Update uniform buffers.
                GlobalUBO ubo{};
                ubo.projectionMatrix = camera.getProjection();
                ubo.viewMatrix = camera.getView();
                ubo.inverseViewMatrix = camera.getInverseView();
                systemSet.pointLightSystem.update(frameInfo, ubo);
                uboBuffers[frameIndex]->writeToBuffer(&ubo);
                uboBuffers[frameIndex]->flush();

                for (auto& keyValue : scene.gameObjects()) {
                    auto& obj = keyValue.second;
                    obj->updateUBO(frameIndex);
                }

                // Dispatch the compute work.

                lthRenderer.beginComputes();
                systemSet.particleSystem.dispatch(frameInfo, computeDescriptorSets[frameIndex]);
                lthRenderer.endComputes();

                // Render the scene.

				lthRenderer.beginSwapChainRenderPass(graphicsCommandBuffer);
                systemSet.renderSystem.render(frameInfo);
                systemSet.pointLightSystem.render(frameInfo);
                systemSet.particleSystem.render(frameInfo);

                // Render the UI.

                ImGui_ImplVulkan_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();
                ImGui::ShowDemoWindow();
                ImGui::Render();
                ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), graphicsCommandBuffer, nullptr);


                // End render pass.

				lthRenderer.endSwapChainRenderPass(graphicsCommandBuffer);
				lthRenderer.endFrame();
			}
        }

		vkDeviceWaitIdle(lthDevice.getDevice());
	}

    // Update the component of the scene according to the different inputs.
    void App::update(float dt) {

        cameraController.moveInPlaneXZ(lthWindow.getGLFWwindow(), dt, viewerTransform);
    }

	void App::loadScene() {
        
        auto viking_room_tex = scene.createTextureFromFile("viking_room.png", true);
        auto circuitry_tex = scene.createTextureFromFile("Circuitry_albedo.bmp", true);

        std::shared_ptr<LthModel> lthModel = scene.createModelFromFile(MODELSFOLDERPATH("smooth_vase.obj"));

        auto smooth_vase = scene.createGameObject();
        smooth_vase->transform.setTranslation({ 0.5f, -0.5f, 0.f });
        smooth_vase->transform.setScale({ 2.f, 2.f, 2.f });
        scene.linkGameObjectToModel(smooth_vase, lthModel);

        lthModel = scene.createModelFromFile(MODELSFOLDERPATH("flat_vase.obj"));

        auto flat_vase = scene.createGameObject();
        flat_vase->transform.setTranslation({ -0.5f, -0.5f, 0.f });
        flat_vase->transform.setScale({ 2.f, 2.f, 2.f });
        scene.linkGameObjectToModel(flat_vase, lthModel);
        flat_vase->setUsesColorTexture(false);
        flat_vase->setTexture(0);

        lthModel = scene.createModelFromFile(MODELSFOLDERPATH("quad.obj"));

        auto floor = scene.createGameObject();
        floor->transform.setTranslation({ 0.f, 0.f, 0.f });
        floor->transform.setScale({ 3.f, 3.f, 3.f});
        scene.linkGameObjectToModel(floor, lthModel);
        floor->setUsesColorTexture(true);
        floor->setTexture(circuitry_tex);

        lthModel = scene.createModelFromFile(MODELSFOLDERPATH("viking_room.obj"));
        //std::shared_ptr<LthTexture> lthTexture = LthTexture::createTextureFromFile(lthDevice, TEXTURESFOLDERPATH("viking_room.png"), true);

        auto viking_room = scene.createGameObject();
        viking_room->transform.setTranslation({ 0.f, 0.f, 5.f });
        scene.linkGameObjectToModel(viking_room, lthModel);
        viking_room->setUsesColorTexture(true);
        viking_room->setTexture(viking_room_tex);

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
            auto pointLight = scene.createPointLight(1.f);
            pointLight->color = lightColors[i];
            auto rotateLight = glm::rotate(
                glm::mat4(1.f),
                (i * TWO_PI) / lightColors.size(),
                { 0.f, -1.f, 0.f });
            pointLight->transform.setTranslation(glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f)));
        }

        scene.createTLAS();
	}

    void App::createDescriptorSets() {
        setLayouts.globalSetLayout = LthDescriptorSetLayout::Builder(lthDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, TEXTUREARRAYSIZE)
            .build();

        setLayouts.gameObjectSetLayout = LthDescriptorSetLayout::Builder(lthDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
            .build();

        setLayouts.computeSetLayout = LthDescriptorSetLayout::Builder(lthDevice)
            //.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .build();


        // General descriptor sets.

        uboBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < uboBuffers.size(); ++i) {
            uboBuffers[i] = std::make_unique<LthBuffer>(
                lthDevice,
                sizeof(GlobalUBO),
                1,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            uboBuffers[i]->map();
        }

        auto descriptorImagesInfo = scene.getDescriptorImagesInfos();

        globalDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < globalDescriptorSets.size(); ++i) {
            VkDescriptorBufferInfo bufferInfo = uboBuffers[i]->descriptorInfo();
            LthDescriptorWriter(*setLayouts.globalSetLayout, *generalDescriptorPool)
                .writeBuffer(0, &bufferInfo)
                .writeImage(1, descriptorImagesInfo.data(), TEXTUREARRAYSIZE)
                .build(globalDescriptorSets[i]);
        }

        // Compute shaders descriptor sets.

        LthParticleSystem::createStorageBuffer(lthDevice, cboBuffers);



        computeDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < computeDescriptorSets.size(); ++i) {
            //VkDescriptorBufferInfo uniformBufferInfo = uboBuffers[i]->descriptorInfo();
            VkDescriptorBufferInfo storageBufferInfoCurrentFrame = cboBuffers[i]->descriptorInfo();
            VkDescriptorBufferInfo storageBufferInfoLastFrame = cboBuffers[(i + MAX_FRAMES_IN_FLIGHT - 1) % MAX_FRAMES_IN_FLIGHT]->descriptorInfo();
            LthDescriptorWriter(*setLayouts.computeSetLayout, *generalDescriptorPool)
                //.writeBuffer(0, &uniformBufferInfo)
                .writeBuffer(0, &storageBufferInfoLastFrame)
                .writeBuffer(1, &storageBufferInfoCurrentFrame)
                .build(computeDescriptorSets[i]);
        }


        // Game objects descriptor sets.

        for (auto& keyValue : scene.gameObjects()) {
            auto& obj = keyValue.second;
            obj->createDescriptorSet(lthDevice, setLayouts.gameObjectSetLayout.get(), generalDescriptorPool.get());
        }
    }

    void App::initImGui() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        ImGui::StyleColorsClassic();

        ImFontConfig fontConfig;
        fontConfig.FontDataOwnedByAtlas = false;
        ImFont* robotoFont = io.Fonts->AddFontFromFileTTF(IMGUIFONTSFOLDERPATH("ProggyClean.ttf"), 15.f);
        io.FontDefault = robotoFont;

        ImGui_ImplGlfw_InitForVulkan(lthWindow.getGLFWwindow(), true);

        ImGui_ImplVulkan_LoadFunctions([](const char* function_name, void* vulkan_instance) {
            return vkGetInstanceProcAddr(*(reinterpret_cast<VkInstance*>(vulkan_instance)), function_name);
            }, (void *) &lthDevice.getInstance());
        ImGui_ImplVulkan_InitInfo initInfo = lthDevice.getImGuiInitInfo(generalDescriptorPool->getDescriptorPool(), static_cast<uint32_t>(lthRenderer.getSwapChainImageCount()));
        ImGui_ImplVulkan_Init(&initInfo, lthRenderer.getSwapChainRenderPass());


        ImGui_ImplVulkan_CreateFontsTexture();
        ImGui_ImplVulkan_DestroyFontsTexture();
    }
}