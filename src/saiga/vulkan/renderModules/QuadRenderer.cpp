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
    uniformBufferVS.destroy();
}
bool QuadRenderer::bind(vk::CommandBuffer cmd)
{
    bindDescriptorSet(cmd, descriptorSet, 0);
    // cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSet, nullptr);
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
    PipelineBase::init(vulkanDevice, 1);

    addDescriptorSetLayout({{0, {7, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex}}, //each ass pair of {index, vk::descriptorsetlayoutbinding}
                            {1, {10, vk::DesriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {2, {11, vk::DesriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {3, {12, vk::DesriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {4, {13, vk::DesriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {5, {14, vk::DesriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment}}
                           });
    addPushConstantRange({vk::ShaderStageFlagBits::eVertex, 0, sizeof(mat4)});
    shaderPipeline.load(device, {vertexShader, fragmentShader});

    PipelineInfo info;
    info.addVertexInfo<VertexNC>();
    create(renderPass, info);



    descriptorSet = createDescriptorSet();
    uniformBufferVS.init(vulkanDevice, &uboVS, sizeof(UBOVS));
    vk::DescriptorBufferInfo descriptorInfo = uniformBufferVS.getDescriptorInfo();
    device.updateDescriptorSets(
        {
            vk::WriteDescriptorSet(descriptorSet, 7, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &descriptorInfo,
                                   nullptr),
        },
        nullptr);


    //setup fullscreen quad
    quad.createFullscreenQuad();
    quad.init(base());
}

void QuadRenderer::render(vk::CommandBuffer cmd){
    pushModel(cmd, identityMat4());
    quad.render(cmd);
}

}  // namespace Vulkan
}  // namespace Saiga
