/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 *
 * Created by Mathias Harrer: mathias.mh.harrer@fau.de
 */

#include "saiga/vulkan/lighting/DeferredLighting.h"

namespace Saiga
{
namespace Vulkan
{
namespace Lighting
{
DeferredLighting::DeferredLighting()
{
    // TODO create lgith meshes here
}
void DeferredLighting::destroy()
{
    debugLightRenderer.destroy();
    pointLightRenderer.destroy();
    spotLightRenderer.destroy();
}

void DeferredLighting::init(Saiga::Vulkan::VulkanBase& vulkanDevice, VkRenderPass renderPass)
{
    const DeferredLightingShaderNames& names = DeferredLightingShaderNames();
    debugLightRenderer.init(vulkanDevice, renderPass, names.debugLightShader, 2);

    pointLightRenderer.init(vulkanDevice, renderPass, names.pointLightShader);
    spotLightRenderer.init(vulkanDevice, renderPass, names.spotLightShader);
}

void DeferredLighting::createAndUpdateDescriptorSets(Memory::ImageMemoryLocation* diffuse,
                                                     Memory::ImageMemoryLocation* specular,
                                                     Memory::ImageMemoryLocation* normal,
                                                     Memory::ImageMemoryLocation* additional,
                                                     Memory::ImageMemoryLocation* depth)
{
    debugLightRenderer.createAndUpdateDescriptorSet(diffuse, specular, normal, additional, depth);
    pointLightRenderer.createAndUpdateDescriptorSet(diffuse, specular, normal, additional, depth);
    spotLightRenderer.createAndUpdateDescriptorSet(diffuse, specular, normal, additional, depth);
}
void DeferredLighting::updateUniformBuffers(vk::CommandBuffer cmd, mat4 proj, mat4 view, bool debugIn)
{
    this->debug = debugIn;
    debugLightRenderer.updateUniformBuffers(cmd, proj, view);

    pointLightRenderer.updateUniformBuffers(cmd, proj, view, debugIn);
    spotLightRenderer.updateUniformBuffers(cmd, proj, view, debugIn);
}

void DeferredLighting::cullLights(Camera* cam)  // TODO broken???
{
    for (auto& l : pointLights)
    {
        l->cullLight(cam);
    }
    for (auto& l : spotLights)
    {
        l->cullLight(cam);
    }
}

void DeferredLighting::renderLights(vk::CommandBuffer cmd, Camera* cam)
{
    cullLights(cam);



    if (debug)
    {
        if (debugLightRenderer.bind(cmd))
        {
            for (std::shared_ptr<PointLight>& l : pointLights)
            {
                if (!l->culled)
                {
                    debugLightRenderer.pushLight(cmd, l);
                    debugLightRenderer.render(cmd, l);
                }
            }
            for (std::shared_ptr<SpotLight>& l : spotLights)
            {
                if (!l->culled)
                {
                    debugLightRenderer.pushLight(cmd, l);
                    debugLightRenderer.render(cmd, l);
                }
            }
        }
    }
    else
    {
        if (pointLightRenderer.bind(cmd))
        {
            for (std::shared_ptr<PointLight>& l : pointLights)
            {
                if (!l->culled)
                {
                    pointLightRenderer.pushLight(cmd, l);
                    pointLightRenderer.render(cmd, l);
                }
            }
        }
        if (spotLightRenderer.bind(cmd))
        {
            for (std::shared_ptr<SpotLight>& l : spotLights)
            {
                if (!l->culled)
                {
                    spotLightRenderer.pushLight(cmd, l);
                    spotLightRenderer.render(cmd, l);
                }
            }
        }
    }
}

void DeferredLighting::reload()
{
    debugLightRenderer.reload();
    spotLightRenderer.reload();
    pointLightRenderer.reload();
}

std::shared_ptr<PointLight> DeferredLighting::createPointLight()
{
    std::shared_ptr<PointLight> l = std::make_shared<PointLight>();
    pointLights.push_back(l);
    return l;
}

std::shared_ptr<SpotLight> DeferredLighting::createSpotLight()
{
    std::shared_ptr<SpotLight> l = std::make_shared<SpotLight>();
    spotLights.push_back(l);
    return l;
}
void DeferredLighting::removeLight(std::shared_ptr<PointLight> l)
{
    pointLights.erase(std::find(pointLights.begin(), pointLights.end(), l));
}

void DeferredLighting::removeLight(std::shared_ptr<SpotLight> l)
{
    spotLights.erase(std::find(spotLights.begin(), spotLights.end(), l));
}

}  // namespace Lighting

}  // namespace Vulkan

}  // namespace Saiga
