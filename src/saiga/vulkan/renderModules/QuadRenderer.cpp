/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#include "QuadRenderer.h"

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
void QuadRenderer::destroy()
{
    Pipeline::destroy();
    // vkDestroySampler(base->device, colorSampler, nullptr);
    uniformBufferVS.destroy();
    // base->device.destroySampler(colorSampler);
}
bool QuadRenderer::bind(vk::CommandBuffer cmd)
{
    std::cout << "QuadRenderer Bind Descriptorset" << std::endl;

    bindDescriptorSet(cmd, descriptorSet, 0);
    // cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSet, nullptr);
    // cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSet, nullptr);
    std::cout << "QuadRenderer call Pipeline::bind(cmd) and return result" << std::endl;

    return Pipeline::bind(cmd);
}

void QuadRenderer::pushModel(VkCommandBuffer cmd, mat4 model)
{
    pushConstant(cmd, vk::ShaderStageFlagBits::eVertex, sizeof(mat4), data(model));
}


void QuadRenderer::updateUniformBuffers(vk::CommandBuffer cmd, mat4 view, mat4 proj)
{
    uboVS.projection = proj;
    uboVS.modelview  = view;
    uboVS.lightPos   = vec4(5, 5, 5, 0);
    uniformBufferVS.update(cmd, sizeof(uboVS), &uboVS);
}

void QuadRenderer::init(VulkanBase& vulkanDevice, VkRenderPass renderPass)
{
    std::cout << "QuadRenderer Init -- START" << std::endl;
    std::cout << "PipelineBase Init -- START" << std::endl;
    PipelineBase::init(vulkanDevice, 1);
    std::cout << "PipelineBase Init -- FINISHED" << std::endl;
    std::cout << "Adding DescriptorSetLayout -- START" << std::endl;

    addDescriptorSetLayout({
        {0,
         {7, vk::DescriptorType::eUniformBuffer, 1,
          vk::ShaderStageFlagBits::eVertex}},  // each pass pair of {index, vk::descriptorsetlayoutbinding}
        {1, {10, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
        {2, {11, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
        {3, {12, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
        {4, {13, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}}
        //{5, {14, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment}}
    });
    std::cout << "Adding DescriptorSetLayout -- FINISHED" << std::endl;

    std::cout << "Adding PushConstantRange -- START" << std::endl;

    addPushConstantRange({vk::ShaderStageFlagBits::eVertex, 0, sizeof(mat4)});
    std::cout << "Adding PushConstantRange -- FINISHED" << std::endl;

    std::cout << "Loading Shader Pipeline -- START" << std::endl;
    shaderPipeline.load(device, {vertexShader, fragmentShader});
    std::cout << "Loading Shader Pipeline -- FINISHED" << std::endl;

    std::cout << "Pipeline Creation -- START" << std::endl;
    PipelineInfo info;
    info.addVertexInfo<VertexNC>();
    info.rasterizationState.cullMode        = vk::CullModeFlagBits::eNone;
    info.blendAttachmentState.blendEnable   = VK_TRUE;
    info.depthStencilState.depthWriteEnable = VK_FALSE;
    std::cout << "create pipeline for QuadRenderer" << std::endl;

    create(renderPass, info);  // descriptorsetlayout is attached to the pipelinelayout here
    std::cout << "Pipeline Creation -- FINISHED" << std::endl;

    // std::cout << "Setup ColorAttachmentSampler -- START" << std::endl;
    // setupColorAttachmentSampler();
    // std::cout << "Setup ColorAttachmentSampler -- FINISHED" << std::endl;



    // setup fullscreen quad
    std::cout << "Fullscreen Quad Creation -- START" << std::endl;
    quad.createFullscreenQuad();
    quad.init(vulkanDevice);
    std::cout << "Fullscreen Quad Creation -- FINISHED" << std::endl;
    std::cout << "QuadRenderer Init -- FINISHED" << std::endl;
}

void QuadRenderer::createAndUpdateDescriptorSet(Saiga::Vulkan::Memory::ImageMemoryLocation* diffuse,
                                                Saiga::Vulkan::Memory::ImageMemoryLocation* specular,
                                                Saiga::Vulkan::Memory::ImageMemoryLocation* normal,
                                                Saiga::Vulkan::Memory::ImageMemoryLocation* additional)
{
    std::cout << "  QuadRenderer DescriptorSet Update/Creation -- START" << std::endl;

    std::cout << "  QuadRenderer DescriptorSet Creation -- START" << std::endl;
    descriptorSet = createDescriptorSet();  // descriptorset is allocated here from a descriptorpool
    std::cout << "  QuadRenderer DescriptorSet Creation -- FINISHED" << std::endl;

    std::cout << "  QuadRenderer uboVS Init -- START" << std::endl;

    uniformBufferVS.init(*base, &uboVS, sizeof(UBOVS));
    std::cout << "  QuadRenderer uboVS Init -- FINISHED" << std::endl;

    vk::DescriptorBufferInfo descriptorInfo = uniformBufferVS.getDescriptorInfo();

    vk::DescriptorImageInfo diffuseDescriptorInfo = diffuse->data.get_descriptor_info();
    diffuseDescriptorInfo.imageLayout             = vk::ImageLayout::eShaderReadOnlyOptimal;

    vk::DescriptorImageInfo specularDescriptorInfo = specular->data.get_descriptor_info();
    specularDescriptorInfo.imageLayout             = vk::ImageLayout::eShaderReadOnlyOptimal;

    vk::DescriptorImageInfo normalDescriptorInfo = normal->data.get_descriptor_info();
    normalDescriptorInfo.imageLayout             = vk::ImageLayout::eShaderReadOnlyOptimal;

    vk::DescriptorImageInfo additionalDescriptorInfo = additional->data.get_descriptor_info();
    additionalDescriptorInfo.imageLayout             = vk::ImageLayout::eShaderReadOnlyOptimal;

    device.updateDescriptorSets(
        {
            vk::WriteDescriptorSet(descriptorSet, 7, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &descriptorInfo,
                                   nullptr),
            vk::WriteDescriptorSet(descriptorSet, 10, 0, 1, vk::DescriptorType::eCombinedImageSampler,
                                   &diffuseDescriptorInfo, nullptr, nullptr),
            vk::WriteDescriptorSet(descriptorSet, 11, 0, 1, vk::DescriptorType::eCombinedImageSampler,
                                   &specularDescriptorInfo, nullptr, nullptr),
            vk::WriteDescriptorSet(descriptorSet, 12, 0, 1, vk::DescriptorType::eCombinedImageSampler,
                                   &normalDescriptorInfo, nullptr, nullptr),
            vk::WriteDescriptorSet(descriptorSet, 13, 0, 1, vk::DescriptorType::eCombinedImageSampler,
                                   &additionalDescriptorInfo, nullptr, nullptr),
            // vk::WriteDescriptorSet(descriptorSet, 14, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr,
            // &descriptorInfo,
            //                     nullptr),
        },
        nullptr);

    std::cout << "  QuadRenderer DescriptorSet Update/Creation -- FINISHED" << std::endl;
}
/*
//!
//! \brief QuadRenderer::setupColorAttachmentSampler
//!
//! creates a sampler to sample from the color attachments of the geometry pass
//!
void QuadRenderer::setupColorAttachmentSampler()
{
    vk::SamplerCreateInfo sampler = vks::initializers::samplerCreateInfo();
    sampler.magFilter             = vk::Filter::eNearest;
    sampler.minFilter             = vk::Filter::eNearest;
    sampler.mipmapMode            = vk::SamplerMipmapMode::eLinear;
    sampler.addressModeU          = vk::SamplerAddressMode::eClampToEdge;
    sampler.addressModeV          = sampler.addressModeU;
    sampler.addressModeW          = sampler.addressModeU;
    sampler.mipLodBias            = 0.0f;
    sampler.maxAnisotropy         = 1.0f;
    sampler.minLod                = 0.0f;
    sampler.maxLod                = 1.0f;
    sampler.borderColor           = vk::BorderColor::eFloatOpaqueWhite;

    base->device.createSampler(&sampler, nullptr, &colorSampler);
    SAIGA_ASSERT(colorSampler);
}
*/

void QuadRenderer::render(vk::CommandBuffer cmd)
{
    bindDescriptorSet(cmd, descriptorSet);
    pushModel(cmd, identityMat4());
    vk::Viewport vp(20.f, 20.f, 200.f, 100.f);
    cmd.setViewport(0, vp);
    quad.render(cmd);
}

}  // namespace Vulkan
}  // namespace Saiga
