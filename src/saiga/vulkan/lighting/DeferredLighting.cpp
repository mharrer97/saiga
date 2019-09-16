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
    attenuatedLightRenderer.destroy();
    pointLightRenderer.destroy();
}

void DeferredLighting::init(Saiga::Vulkan::VulkanBase& vulkanDevice, VkRenderPass renderPass)
{
    attenuatedLightRenderer.init(vulkanDevice, renderPass);
    pointLightRenderer.init(vulkanDevice, renderPass);
}

void DeferredLighting::createAndUpdateDescriptorSets(Memory::ImageMemoryLocation* diffuse,
                                                     Memory::ImageMemoryLocation* specular,
                                                     Memory::ImageMemoryLocation* normal,
                                                     Memory::ImageMemoryLocation* additional,
                                                     Memory::ImageMemoryLocation* depth)
{
    attenuatedLightRenderer.createAndUpdateDescriptorSet(diffuse, specular, normal, additional, depth);
    pointLightRenderer.createAndUpdateDescriptorSet(diffuse, specular, normal, additional, depth);
}
void DeferredLighting::updateUniformBuffers(vk::CommandBuffer cmd, mat4 proj, mat4 view)
{
    attenuatedLightRenderer.updateUniformBuffers(cmd, proj, view, vec4(-5.f, 5.f, 5.f, 1.f), 10.f, false);
    pointLightRenderer.updateUniformBuffers(cmd, proj, view, false);
}

void DeferredLighting::renderLights(vk::CommandBuffer cmd)
{
    if (attenuatedLightRenderer.bind(cmd))
    {
        for (auto& l : attenuatedLights)
        {
            vec4 pos = vec4(l->position[0], l->position[1], l->position[2], 1.f);
            attenuatedLightRenderer.pushPosition(cmd, pos);
            attenuatedLightRenderer.render(cmd, l);
        }
    }

    if (pointLightRenderer.bind(cmd))
    {
        for (auto& l : pointLights)
        {
            pointLightRenderer.pushLight(cmd, l);
            pointLightRenderer.render(cmd, l);
        }
    }
}

void DeferredLighting::reload()
{
    attenuatedLightRenderer.reload();
}

std::shared_ptr<PointLight> DeferredLighting::createPointLight()
{
    std::shared_ptr<PointLight> l = std::make_shared<PointLight>();
    pointLights.push_back(l);
    return l;
}

void DeferredLighting::removeLight(std::shared_ptr<PointLight> l)
{
    pointLights.erase(std::find(pointLights.begin(), pointLights.end(), l));
}


// TODO delete
std::shared_ptr<AttenuatedLight> DeferredLighting::createAttenuatedLight()
{
    std::shared_ptr<AttenuatedLight> l = std::make_shared<AttenuatedLight>();
    attenuatedLights.push_back(l);
    return l;
}

void DeferredLighting::removeLight(std::shared_ptr<AttenuatedLight> l)
{
    attenuatedLights.erase(std::find(attenuatedLights.begin(), attenuatedLights.end(), l));
}
}  // namespace Lighting

}  // namespace Vulkan

}  // namespace Saiga
