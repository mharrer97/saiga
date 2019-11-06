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
#include "saiga/vulkan/window/SDLSample.h"

#include <vector>
using namespace Saiga;
class VulkanExample : public VulkanDeferredSDLExampleBase
{
   public:
    // VulkanExample(Saiga::Vulkan::VulkanWindow& window, Saiga::Vulkan::VulkanDeferredRenderer& renderer);
    VulkanExample();
    ~VulkanExample() override;

    void init(Saiga::Vulkan::VulkanBase& base);


    void update(float dt) override;
    void transfer(vk::CommandBuffer cmd, Camera* cam) override;
    void transferDepth(vk::CommandBuffer cmd, Camera* cam) override;
    void transferForward(vk::CommandBuffer cmd, Camera* cam) override;
    void render(vk::CommandBuffer cmd, Camera* cam) override;
    void renderDepth(vk::CommandBuffer cmd, Camera* cam) override;
    void renderForward(vk::CommandBuffer cmd, Camera* cam) override;
    void renderGUI() override;

   private:
    std::vector<vec3> boxOffsets;
    // Saiga::SDLCamera<Saiga::PerspectiveCamera> camera;
    bool uploadChanges = true;

    float lightRadius           = 45.f;
    float spotLightOpeningAngle = 134.75f;
    float lightHeight           = 18.f;

    Saiga::Vulkan::UniversalShadowAssetRenderer assetRenderer;
    Saiga::Vulkan::UniversalShadowLineAssetRenderer lineAssetRenderer;
    Saiga::Vulkan::UniversalShadowPointCloudRenderer pointCloudRenderer;
    Saiga::Vulkan::UniversalShadowTexturedAssetRenderer texturedAssetRenderer;

    Saiga::Vulkan::VulkanVertexColoredAsset cornell;
    Saiga::Vulkan::VulkanVertexColoredAsset sphere;

    // std::vector<std::shared_ptr<Saiga::Vulkan::Lighting::PointLight>> pointLights;
    std::shared_ptr<Saiga::Vulkan::Lighting::SpotLight> spotLight;
    // std::shared_ptr<Saiga::Vulkan::Lighting::BoxLight> boxLight;
    std::shared_ptr<Saiga::Vulkan::Lighting::DirectionalLight> directionalLight;


    bool displayModels = true;
};
