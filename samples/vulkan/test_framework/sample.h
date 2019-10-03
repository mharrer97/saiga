/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 *
 * Created by Mathias Harrer: mathias.mh.harrer@fau.de
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
    void transfer(vk::CommandBuffer cmd, Camera* cam) override;
    void transferForward(vk::CommandBuffer cmd, Camera* cam) override;
    void render(vk::CommandBuffer cmd, Camera* cam) override;
    void renderForward(vk::CommandBuffer cmd, Camera* cam) override;
    void renderGUI() override;

   private:
    std::vector<vec3> boxOffsets;
    Saiga::SDLCamera<Saiga::PerspectiveCamera> camera;
    bool change        = false;
    bool uploadChanges = true;
    Saiga::Object3D teapotTrans;

    float timingLoop  = 0.f;  // used for rotating camera and light etc;
    float timingLoop2 = 0.f;  // used for animating candle etc;

    float lightRadius           = 10.f;
    float spotLightOpeningAngle = 90.f;

    std::shared_ptr<Saiga::Vulkan::Texture2D> texture;

    Saiga::Vulkan::VulkanTexturedAsset box;
    Saiga::Vulkan::VulkanVertexColoredAsset teapot, plane, sphere, candle;
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

    std::vector<std::shared_ptr<Saiga::Vulkan::Lighting::PointLight>> pointLights;
    std::shared_ptr<Saiga::Vulkan::Lighting::SpotLight> spotLight, candleLight;


    bool displayModels = true;


    void keyPressed(SDL_Keysym key) override;
    void keyReleased(SDL_Keysym key) override;
};
