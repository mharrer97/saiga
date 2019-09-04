/*
 * Vulkan Example - imGui (https://github.com/ocornut/imgui)
 *
 * Copyright (C) 2017 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "saiga/core/sdl/sdl_camera.h"
#include "saiga/core/sdl/sdl_eventhandler.h"
#include "saiga/core/window/Interfaces.h"
#include "saiga/vulkan/VulkanDeferredRenderer.h"
#include "saiga/vulkan/memory/VulkanMemory.h"
#include "saiga/vulkan/pipeline/DescriptorSet.h"
#include "saiga/vulkan/renderModules/AssetRenderer.h"
#include "saiga/vulkan/renderModules/LineAssetRenderer.h"
#include "saiga/vulkan/renderModules/PointCloudRenderer.h"
#include "saiga/vulkan/renderModules/TextureDisplay.h"
#include "saiga/vulkan/renderModules/TexturedAssetRenderer.h"
#include "saiga/vulkan/renderModules/UniversalRenderer.h"

#include <vector>
using namespace Saiga;
class VulkanExample : public Saiga::Updating,
                      public Saiga::Vulkan::VulkanDeferredRenderingInterface,
                      public Saiga::SDL_KeyListener
{
   public:
    VulkanExample(Saiga::Vulkan::VulkanWindow& window, Saiga::Vulkan::VulkanDeferredRenderer& renderer);
    ~VulkanExample() override;

    void init(Saiga::Vulkan::VulkanBase& base);


    void update(float dt) override;
    void transfer(vk::CommandBuffer cmd) override;
    void transferForward(vk::CommandBuffer cmd) override;
    void render(vk::CommandBuffer cmd) override;
    void renderForward(vk::CommandBuffer cmd) override;
    mat4 getCameraView() override;  // TODO eleganter loesen
    void renderGUI() override;

   private:
    std::vector<vec3> boxOffsets;
    Saiga::SDLCamera<Saiga::PerspectiveCamera> camera;
    bool change        = false;
    bool uploadChanges = true;
    Saiga::Object3D teapotTrans;

    std::shared_ptr<Saiga::Vulkan::Texture2D> texture;

    Saiga::Vulkan::VulkanTexturedAsset box;
    Saiga::Vulkan::VulkanVertexColoredAsset teapot, plane;
    Saiga::Vulkan::VulkanLineVertexColoredAsset grid, frustum;
    Saiga::Vulkan::VulkanPointCloudAsset pointCloud;
    Saiga::Vulkan::UniversalAssetRenderer assetRenderer;
    Saiga::Vulkan::UniversalLineAssetRenderer lineAssetRenderer;
    Saiga::Vulkan::UniversalPointCloudRenderer pointCloudRenderer;
    Saiga::Vulkan::UniversalTexturedAssetRenderer texturedAssetRenderer;

    //
    Saiga::Vulkan::StaticDescriptorSet textureDes;
    Saiga::Vulkan::UniversalTextureDisplay textureDisplay;

    Saiga::Vulkan::VulkanDeferredRenderer& renderer;

    bool displayModels = true;


    void keyPressed(SDL_Keysym key) override;
    void keyReleased(SDL_Keysym key) override;
};
