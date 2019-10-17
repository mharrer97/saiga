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
void DeferredLighting::setupShadowPass()
{
    // create shadow Pass

    vk::AttachmentDescription attachment;
    // Depth attachment
    attachment.format         = shadowMapFormat;
    attachment.samples        = vk::SampleCountFlagBits::e1;
    attachment.loadOp         = vk::AttachmentLoadOp::eClear;
    attachment.storeOp        = vk::AttachmentStoreOp::eStore;
    attachment.stencilLoadOp  = vk::AttachmentLoadOp::eClear;
    attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    attachment.initialLayout  = vk::ImageLayout::eUndefined;
    attachment.finalLayout    = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::AttachmentReference depthReference = {};
    depthReference.attachment              = 0;
    depthReference.layout                  = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::SubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint      = vk::PipelineBindPoint::eGraphics;
    subpassDescription.colorAttachmentCount   = 0;
    // subpassDescription.pColorAttachments       = VK_ATTACHMENT_UNUSED;
    subpassDescription.pDepthStencilAttachment = &depthReference;
    subpassDescription.inputAttachmentCount    = 0;
    subpassDescription.pInputAttachments       = nullptr;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments    = nullptr;
    subpassDescription.pResolveAttachments     = nullptr;

    // Subpass dependencies for layout transitions
    std::array<vk::SubpassDependency, 2> dependencies;

    dependencies[0].srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass    = 0;
    dependencies[0].srcStageMask  = vk::PipelineStageFlagBits::eBottomOfPipe;
    dependencies[0].dstStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependencies[0].srcAccessMask = vk::AccessFlagBits::eMemoryRead;
    dependencies[0].dstAccessMask =
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
    dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

    dependencies[1].srcSubpass   = 0;
    dependencies[1].dstSubpass   = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependencies[1].dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
    dependencies[1].srcAccessMask =
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
    dependencies[1].dstAccessMask   = vk::AccessFlagBits::eMemoryRead;
    dependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

    vk::RenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.attachmentCount          = 1;  // static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments             = &attachment;
    renderPassInfo.subpassCount             = 1;
    renderPassInfo.pSubpasses               = &subpassDescription;
    renderPassInfo.dependencyCount          = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies            = dependencies.data();

    base->device.createRenderPass(&renderPassInfo, nullptr, &shadowPass);
    SAIGA_ASSERT(shadowPass);
}

DeferredLighting::DeferredLighting()
{
    // TODO create lgith meshes here
}

void DeferredLighting::destroy()
{
    debugLightRenderer.destroy();
    directionalLightRenderer.destroy();
    pointLightRenderer.destroy();
    spotLightRenderer.destroy();
    boxLightRenderer.destroy();
    base->device.destroyRenderPass(shadowPass);
}

void DeferredLighting::init(Saiga::Vulkan::VulkanBase& vulkanDevice, vk::RenderPass renderPass)
{
    this->base = &vulkanDevice;

    setupShadowPass();

    const DeferredLightingShaderNames& names = DeferredLightingShaderNames();
    debugLightRenderer.init(vulkanDevice, renderPass, names.debugLightShader, 2);

    directionalLightRenderer.init(vulkanDevice, renderPass, names.directionalLightShader);
    pointLightRenderer.init(vulkanDevice, renderPass, names.pointLightShader);
    spotLightRenderer.init(vulkanDevice, renderPass, names.spotLightShader);
    boxLightRenderer.init(vulkanDevice, renderPass, names.boxLightShader);
}

void DeferredLighting::createAndUpdateDescriptorSets(Memory::ImageMemoryLocation* diffuse,
                                                     Memory::ImageMemoryLocation* specular,
                                                     Memory::ImageMemoryLocation* normal,
                                                     Memory::ImageMemoryLocation* additional,
                                                     Memory::ImageMemoryLocation* depth)
{
    debugLightRenderer.createAndUpdateDescriptorSet(diffuse, specular, normal, additional, depth);
    directionalLightRenderer.createAndUpdateDescriptorSet(diffuse, specular, normal, additional, depth);
    pointLightRenderer.createAndUpdateDescriptorSet(diffuse, specular, normal, additional, depth);
    spotLightRenderer.createAndUpdateDescriptorSet(diffuse, specular, normal, additional, depth);
    boxLightRenderer.createAndUpdateDescriptorSet(diffuse, specular, normal, additional, depth);
}
void DeferredLighting::updateUniformBuffers(vk::CommandBuffer cmd, mat4 proj, mat4 view, bool debugIn)
{
    this->debug = debugIn;
    debugLightRenderer.updateUniformBuffers(cmd, proj, view);

    directionalLightRenderer.updateUniformBuffers(cmd, proj, view, debugIn);
    pointLightRenderer.updateUniformBuffers(cmd, proj, view, debugIn);
    spotLightRenderer.updateUniformBuffers(cmd, proj, view, debugIn);
    boxLightRenderer.updateUniformBuffers(cmd, proj, view, debugIn);
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
    for (auto& l : boxLights)
    {
        l->calculateCamera();
        l->shadowCamera.recalculatePlanes();
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
            for (std::shared_ptr<BoxLight>& l : boxLights)
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
        if (boxLightRenderer.bind(cmd))
        {
            for (std::shared_ptr<BoxLight>& l : boxLights)
            {
                if (!l->culled)
                {
                    boxLightRenderer.pushLight(cmd, l, cam);
                    boxLightRenderer.render(cmd, l);
                }
            }
        }
    }
    if (directionalLightRenderer.bind(cmd))
    {
        for (std::shared_ptr<DirectionalLight>& l : directionalLights)
        {
            directionalLightRenderer.pushLight(cmd, l);
            directionalLightRenderer.render(cmd);
        }
    }
}


void DeferredLighting::renderDepthMaps(VulkanDeferredRenderingInterface* renderer)
{
    // render to depthmaps of all needed lights that have shadows. use renderdepth of renderer to get access to the
    // sample
}
void DeferredLighting::reload()
{
    debugLightRenderer.reload();
    directionalLightRenderer.reload();
    spotLightRenderer.reload();
    pointLightRenderer.reload();
    boxLightRenderer.reload();
}

std::shared_ptr<DirectionalLight> DeferredLighting::createDirectionalLight()
{
    std::shared_ptr<DirectionalLight> l = std::make_shared<DirectionalLight>();
    directionalLights.push_back(l);
    return l;
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

std::shared_ptr<BoxLight> DeferredLighting::createBoxLight()
{
    std::shared_ptr<BoxLight> l = std::make_shared<BoxLight>();
    boxLights.push_back(l);
    return l;
}

void DeferredLighting::enableShadowMapping(std::shared_ptr<DirectionalLight> l)
{
    // TODO shadowmap creation here?
    if (!l->hasShadows())
    {
        l->createShadowMap(*base, 100, 100, shadowPass);
        l->enableShadows();
    }
}

void DeferredLighting::removeLight(std::shared_ptr<DirectionalLight> l)
{
    directionalLights.erase(std::find(directionalLights.begin(), directionalLights.end(), l));
}
void DeferredLighting::removeLight(std::shared_ptr<PointLight> l)
{
    pointLights.erase(std::find(pointLights.begin(), pointLights.end(), l));
}

void DeferredLighting::removeLight(std::shared_ptr<SpotLight> l)
{
    spotLights.erase(std::find(spotLights.begin(), spotLights.end(), l));
}

void DeferredLighting::removeLight(std::shared_ptr<BoxLight> l)
{
    boxLights.erase(std::find(boxLights.begin(), boxLights.end(), l));
}
}  // namespace Lighting

}  // namespace Vulkan

}  // namespace Saiga
