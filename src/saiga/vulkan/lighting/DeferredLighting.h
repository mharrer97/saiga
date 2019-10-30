/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 *
 * Created by Mathias Harrer: mathias.mh.harrer@fau.de
 */


#pragma once
#include "saiga/core/camera/camera.h"
#include "saiga/vulkan/VulkanAsset.h"
#include "saiga/vulkan/lighting/BoxLight.h"
#include "saiga/vulkan/lighting/DebugLight.h"
#include "saiga/vulkan/lighting/DirectionalLight.h"
#include "saiga/vulkan/lighting/PointLight.h"
#include "saiga/vulkan/lighting/SpotLight.h"

namespace Saiga
{
namespace Vulkan
{
class VulkanDeferredRenderingInterface;

namespace Lighting
{
struct DeferredLightingShaderNames
{
    std::string attenuatedLightShader        = "vulkan/lighting/attenuatedLight.frag";
    std::string pointLightShader             = "vulkan/lighting/pointLight.frag";
    std::string spotLightShader              = "vulkan/lighting/spotLight.frag";
    std::string boxLightShader               = "vulkan/lighting/boxLight.frag";
    std::string directionalLightShader       = "vulkan/lighting/directionalLight.frag";
    std::string debugLightShader             = "vulkan/lighting/debugLight.frag";
    std::string directionalShadowLightShader = "vulkan/lighting/directionalShadowLight.frag";
};

class SAIGA_VULKAN_API DeferredLighting
{
   private:
    VulkanBase* base;
    vk::Format shadowMapFormat = vk::Format::eD32Sfloat;


    DebugLightRenderer debugLightRenderer;

    DirectionalLightRenderer directionalLightRenderer;
    DirectionalShadowLightRenderer directionalShadowLightRenderer;
    std::vector<std::shared_ptr<DirectionalLight>> directionalLights;

    PointLightRenderer pointLightRenderer;
    std::vector<std::shared_ptr<PointLight>> pointLights;

    SpotLightRenderer spotLightRenderer;
    std::vector<std::shared_ptr<SpotLight>> spotLights;

    BoxLightRenderer boxLightRenderer;
    std::vector<std::shared_ptr<BoxLight>> boxLights;

    void setupShadowPass();

   public:
    vk::RenderPass shadowPass;

    int totalLights;
    int visibleLights;

    bool debug                   = false;
    bool renderPointLights       = true;
    bool renderSpotLights        = true;
    bool renderBoxLights         = true;
    bool renderDirectionalLights = true;


    DeferredLighting();
    ~DeferredLighting() { destroy(); }
    DeferredLighting& operator=(DeferredLighting& l) = delete;
    void destroy();

    void init(Saiga::Vulkan::VulkanBase& vulkanDevice, vk::RenderPass renderPass);
    void createAndUpdateDescriptorSets(Saiga::Vulkan::Memory::ImageMemoryLocation* diffuse,
                                       Saiga::Vulkan::Memory::ImageMemoryLocation* specular,
                                       Saiga::Vulkan::Memory::ImageMemoryLocation* normal,
                                       Saiga::Vulkan::Memory::ImageMemoryLocation* additional,
                                       Saiga::Vulkan::Memory::ImageMemoryLocation* depth);
    void updateUniformBuffers(vk::CommandBuffer cmd, mat4 proj, mat4 view, bool debug);

    void cullLights(Camera* cam);
    void renderLights(vk::CommandBuffer cmd, Camera* cam);

    void renderDepthMaps(vk::CommandBuffer cmd, VulkanDeferredRenderingInterface* renderer);

    void reload();

    // used for setting each render flag for each light type
    void setRenderPointLights(bool rP) { renderPointLights = rP; }
    void setRenderSpotLights(bool rS) { renderSpotLights = rS; }
    void setRenderBoxLights(bool rB) { renderBoxLights = rB; }
    void setRenderDirectionalLights(bool rD) { renderDirectionalLights = rD; }

    // setRenderLights does not effect directional lights
    void setRenderLights(bool rL)
    {
        renderPointLights = rL;
        renderSpotLights  = rL;
        renderBoxLights   = rL;
    }

    std::shared_ptr<DirectionalLight> createDirectionalLight();
    std::shared_ptr<PointLight> createPointLight();
    std::shared_ptr<SpotLight> createSpotLight();
    std::shared_ptr<BoxLight> createBoxLight();

    void enableShadowMapping(std::shared_ptr<DirectionalLight> l);

    void removeLight(std::shared_ptr<DirectionalLight> l);
    void removeLight(std::shared_ptr<PointLight> l);
    void removeLight(std::shared_ptr<SpotLight> l);
    void removeLight(std::shared_ptr<BoxLight> l);
};
}  // namespace Lighting
}  // namespace Vulkan
}  // namespace Saiga
