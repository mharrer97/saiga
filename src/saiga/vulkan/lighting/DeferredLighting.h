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
namespace Lighting
{
struct DeferredLightingShaderNames
{
    std::string attenuatedLightShader  = "vulkan/lighting/attenuatedLight.frag";
    std::string pointLightShader       = "vulkan/lighting/pointLight.frag";
    std::string spotLightShader        = "vulkan/lighting/spotLight.frag";
    std::string boxLightShader         = "vulkan/lighting/boxLight.frag";
    std::string directionalLightShader = "vulkan/lighting/directionalLight.frag";
    std::string debugLightShader       = "vulkan/lighting/debugLight.frag";
};

class SAIGA_VULKAN_API DeferredLighting
{
   private:
    DebugLightRenderer debugLightRenderer;

    DirectionalLightRenderer directionalLightRenderer;
    std::vector<std::shared_ptr<DirectionalLight>> directionalLights;

    PointLightRenderer pointLightRenderer;
    std::vector<std::shared_ptr<PointLight>> pointLights;

    SpotLightRenderer spotLightRenderer;
    std::vector<std::shared_ptr<SpotLight>> spotLights;

    BoxLightRenderer boxLightRenderer;
    std::vector<std::shared_ptr<BoxLight>> boxLights;


   public:
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

    void init(Saiga::Vulkan::VulkanBase& vulkanDevice, VkRenderPass renderPass);
    void createAndUpdateDescriptorSets(Saiga::Vulkan::Memory::ImageMemoryLocation* diffuse,
                                       Saiga::Vulkan::Memory::ImageMemoryLocation* specular,
                                       Saiga::Vulkan::Memory::ImageMemoryLocation* normal,
                                       Saiga::Vulkan::Memory::ImageMemoryLocation* additional,
                                       Saiga::Vulkan::Memory::ImageMemoryLocation* depth);
    void updateUniformBuffers(vk::CommandBuffer cmd, mat4 proj, mat4 view, bool debug);

    void cullLights(Camera* cam);
    void renderLights(vk::CommandBuffer cmd, Camera* cam);

    void reload();

    std::shared_ptr<DirectionalLight> createDirectionalLight();
    std::shared_ptr<PointLight> createPointLight();
    std::shared_ptr<SpotLight> createSpotLight();
    std::shared_ptr<BoxLight> createBoxLight();


    void removeLight(std::shared_ptr<DirectionalLight> l);
    void removeLight(std::shared_ptr<PointLight> l);
    void removeLight(std::shared_ptr<SpotLight> l);
    void removeLight(std::shared_ptr<BoxLight> l);
};
}  // namespace Lighting
}  // namespace Vulkan
}  // namespace Saiga
