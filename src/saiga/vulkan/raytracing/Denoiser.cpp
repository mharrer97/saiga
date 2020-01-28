/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 *
 * Created by Mathias Harrer: mathias.mh.harrer@fau.de
 */

#include "Denoiser.h"

#include "saiga/export.h"

namespace Saiga
{
namespace Vulkan
{
namespace RTX
{
void Denoiser::destroy()
{
    Pipeline::destroy();
    uniformBufferFS.destroy();
}

void Denoiser::render(vk::CommandBuffer cmd)
{
    bindDescriptorSet(cmd, descriptorSet);
    blitMesh.render(cmd);
}

void Denoiser::init(Saiga::Vulkan::VulkanBase& vulkanDevice, VkRenderPass renderPass)
{
    PipelineBase::init(vulkanDevice, 1);
    addDescriptorSetLayout({{0, {11, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {1, {12, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {2, {13, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {3, {14, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment}}

    });

    shaderPipeline.load(device, {vertexShader, fragmentShader});
    PipelineInfo info;
    info.addVertexInfo<VertexType>();
    info.blendAttachmentState.blendEnable         = VK_TRUE;
    info.blendAttachmentState.alphaBlendOp        = vk::BlendOp::eAdd;
    info.blendAttachmentState.colorBlendOp        = vk::BlendOp::eAdd;
    info.blendAttachmentState.dstColorBlendFactor = vk::BlendFactor::eOne;
    info.blendAttachmentState.srcColorBlendFactor = vk::BlendFactor::eOne;
    info.blendAttachmentState.dstAlphaBlendFactor = vk::BlendFactor::eOne;
    info.blendAttachmentState.srcAlphaBlendFactor = vk::BlendFactor::eOne;
    info.rasterizationState.cullMode              = vk::CullModeFlagBits::eNone;


    create(renderPass, info);

    blitMesh.createFullscreenQuad();
    blitMesh.init(vulkanDevice);
}

void Denoiser::updateUniformBuffers(vk::CommandBuffer cmd, int maxKernelSize, int width, int height)
{
    uboFS.maxKernelSize = maxKernelSize;
    uboFS.width         = width;
    uboFS.height        = height;
    uniformBufferFS.update(cmd, sizeof(uboFS), &uboFS);
}

void Denoiser::createAndUpdateDescriptorSet(Saiga::Vulkan::Memory::ImageMemoryLocation* rtx,
                                            Saiga::Vulkan::Memory::ImageMemoryLocation* normal,
                                            Saiga::Vulkan::Memory::ImageMemoryLocation* data)
{
    descriptorSet = createDescriptorSet();

    vk::DescriptorImageInfo rtxDescriptorInfo = rtx->data.get_descriptor_info();
    rtxDescriptorInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    rtxDescriptorInfo.setImageView(rtx->data.view);
    rtxDescriptorInfo.setSampler(rtx->data.sampler);

    vk::DescriptorImageInfo normalDescriptorInfo = normal->data.get_descriptor_info();
    normalDescriptorInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    normalDescriptorInfo.setImageView(normal->data.view);
    normalDescriptorInfo.setSampler(normal->data.sampler);

    vk::DescriptorImageInfo dataDescriptorInfo = data->data.get_descriptor_info();
    dataDescriptorInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    dataDescriptorInfo.setImageView(data->data.view);
    dataDescriptorInfo.setSampler(data->data.sampler);

    uniformBufferFS.init(*base, &uboFS, sizeof(uboFS));
    vk::DescriptorBufferInfo uboFSDescriptorInfo = uniformBufferFS.getDescriptorInfo();

    device.updateDescriptorSets(
        {
            vk::WriteDescriptorSet(descriptorSet, 11, 0, 1, vk::DescriptorType::eCombinedImageSampler,
                                   &rtxDescriptorInfo, nullptr, nullptr),
            vk::WriteDescriptorSet(descriptorSet, 12, 0, 1, vk::DescriptorType::eCombinedImageSampler,
                                   &normalDescriptorInfo, nullptr, nullptr),
            vk::WriteDescriptorSet(descriptorSet, 13, 0, 1, vk::DescriptorType::eCombinedImageSampler,
                                   &dataDescriptorInfo, nullptr, nullptr),
            vk::WriteDescriptorSet(descriptorSet, 14, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr,
                                   &uboFSDescriptorInfo, nullptr),

        },
        nullptr);
}


}  // namespace RTX
}  // namespace Vulkan
}  // namespace Saiga
