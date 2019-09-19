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
    std::string pointLightShader = "vulkan/lighting/lightPoint.glsl";
};

class SAIGA_VULKAN_API DeferredLighting
{
   private:
    PointLightRenderer pointLightRenderer;
    std::vector<std::shared_ptr<PointLight>> pointLights;

    SpotLightRenderer spotLightRenderer;
    std::vector<std::shared_ptr<SpotLight>> spotLights;

    std::vector<std::shared_ptr<AttenuatedLight>> attenuatedLights;


   public:
    int totalLights;
    int visibleLights;

    bool renderPointLights = true;
    bool renderSpotLights  = true;


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
    void renderLights(vk::CommandBuffer cmd);

    void reload();

    std::shared_ptr<PointLight> createPointLight();
    std::shared_ptr<SpotLight> createSpotLight();

    // TODO delete
    std::shared_ptr<AttenuatedLight> createAttenuatedLight();
    void removeLight(std::shared_ptr<AttenuatedLight> l);
    AttenuatedLightRenderer attenuatedLightRenderer;

    void removeLight(std::shared_ptr<PointLight> l);
    void removeLight(std::shared_ptr<SpotLight> l);
};
}  // namespace Lighting
}  // namespace Vulkan
}  // namespace Saiga
