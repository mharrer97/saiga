/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#include "TestRenderer.h"

#include "saiga/core/model/objModelLoader.h"
#include "saiga/vulkan/Shader/all.h"
#include "saiga/vulkan/Vertex.h"

#if defined(SAIGA_OPENGL_INCLUDED)
#    error OpenGL was included somewhere.
#endif

namespace Saiga
{
namespace Vulkan
{
void TestRenderer::destroy()
{
    Pipeline::destroy();
}

void TestRenderer::render(vk::CommandBuffer cmd, DescriptorSet& descriptor, vec2 position, vec2 size)
{
    bindDescriptorSet(cmd, descriptor);
    vk::Viewport vp(position[0], position[1], size[0], size[1]);
    cmd.setViewport(0, vp);
    blitMesh.render(cmd);
}



void TestRenderer::init(VulkanBase& vulkanDevice, VkRenderPass renderPass)
{
    PipelineBase::init(vulkanDevice, 1);
    addDescriptorSetLayout(
        {{0, {11, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
         {1, {12, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
         {2, {13, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
         {3, {14, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
         {4, {15, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}}});

    addPushConstantRange({vk::ShaderStageFlagBits::eVertex, 0, sizeof(mat4)});
    shaderPipeline.load(device, {"vulkan/testRenderer.vert", "vulkan/testRenderer.frag"});
    PipelineInfo info;
    info.addVertexInfo<VertexType>();
    info.rasterizationState.cullMode = vk::CullModeFlagBits::eNone;
    //    info.blendAttachmentState.blendEnable   = VK_TRUE;
    // info.depthStencilState.depthWriteEnable = VK_TRUE;
    // info.depthStencilState.depthTestEnable  = VK_FALSE;

    create(renderPass, info);

    blitMesh.createFullscreenQuad();
    blitMesh.init(vulkanDevice);
}

StaticDescriptorSet TestRenderer::createAndUpdateDescriptorSet(Saiga::Vulkan::Memory::ImageMemoryLocation* diffuse,
                                                               Saiga::Vulkan::Memory::ImageMemoryLocation* specular,
                                                               Saiga::Vulkan::Memory::ImageMemoryLocation* normal,
                                                               Saiga::Vulkan::Memory::ImageMemoryLocation* additional,
                                                               Saiga::Vulkan::Memory::ImageMemoryLocation* depth)
{
    auto set = createDescriptorSet();


    vk::DescriptorImageInfo diffuseDescriptorInfo = diffuse->data.get_descriptor_info();
    diffuseDescriptorInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    diffuseDescriptorInfo.setImageView(diffuse->data.view);
    diffuseDescriptorInfo.setSampler(diffuse->data.sampler);

    vk::DescriptorImageInfo specularDescriptorInfo = specular->data.get_descriptor_info();
    specularDescriptorInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    specularDescriptorInfo.setImageView(specular->data.view);
    specularDescriptorInfo.setSampler(specular->data.sampler);

    vk::DescriptorImageInfo normalDescriptorInfo = normal->data.get_descriptor_info();
    normalDescriptorInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    normalDescriptorInfo.setImageView(normal->data.view);
    normalDescriptorInfo.setSampler(normal->data.sampler);

    vk::DescriptorImageInfo additionalDescriptorInfo = additional->data.get_descriptor_info();
    additionalDescriptorInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    additionalDescriptorInfo.setImageView(additional->data.view);
    additionalDescriptorInfo.setSampler(additional->data.sampler);

    vk::DescriptorImageInfo depthDescriptorInfo = depth->data.get_descriptor_info();
    depthDescriptorInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    depthDescriptorInfo.setImageView(depth->data.view);
    depthDescriptorInfo.setSampler(depth->data.sampler);


    device.updateDescriptorSets(
        {
            vk::WriteDescriptorSet(set, 11, 0, 1, vk::DescriptorType::eCombinedImageSampler, &diffuseDescriptorInfo,
                                   nullptr, nullptr),
            vk::WriteDescriptorSet(set, 12, 0, 1, vk::DescriptorType::eCombinedImageSampler, &specularDescriptorInfo,
                                   nullptr, nullptr),
            vk::WriteDescriptorSet(set, 13, 0, 1, vk::DescriptorType::eCombinedImageSampler, &normalDescriptorInfo,
                                   nullptr, nullptr),
            vk::WriteDescriptorSet(set, 14, 0, 1, vk::DescriptorType::eCombinedImageSampler, &additionalDescriptorInfo,
                                   nullptr, nullptr),
            vk::WriteDescriptorSet(set, 15, 0, 1, vk::DescriptorType::eCombinedImageSampler, &depthDescriptorInfo,
                                   nullptr, nullptr),
        },
        nullptr);
    return set;
}

}  // namespace Vulkan
}  // namespace Saiga
