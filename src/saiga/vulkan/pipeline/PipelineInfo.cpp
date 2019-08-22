/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#include "PipelineInfo.h"

namespace Saiga
{
namespace Vulkan
{
void PipelineInfo::addShaders(GraphicsShaderPipeline& shaders)
{
    //    shaderStages.clear();
    //    for (auto& s : shaders.modules)
    //    {
    //        shaderStages.push_back(s.createPipelineInfo());
    //    }
}

vk::GraphicsPipelineCreateInfo PipelineInfo::createCreateInfo(vk::PipelineLayout pipelineLayout,
                                                              vk::RenderPass renderPass, int colorAttachmentCount)
{
    dynamicState.dynamicStateCount = dynamicStateEnables.size();
    dynamicState.pDynamicStates    = dynamicStateEnables.data();


    vi.vertexBindingDescriptionCount   = 1;
    vi.pVertexBindingDescriptions      = &vertexInputBindings;
    vi.vertexAttributeDescriptionCount = vertexInputAttributes.size();
    vi.pVertexAttributeDescriptions    = vertexInputAttributes.data();

    blendAttachmentStates.clear();
    for (int i = 0; i < colorAttachmentCount; ++i)
    {
        /*vk::PipelineColorBlendAttachmentState blendAttachmentState = {false,
                                                        vk::BlendFactor::eSrcAlpha,
                                                        vk::BlendFactor::eOneMinusSrcAlpha,
                                                        vk::BlendOp::eAdd,
                                                        vk::BlendFactor::eOneMinusSrcAlpha,
                                                        vk::BlendFactor::eZero,
                                                        vk::BlendOp::eAdd,
                                                        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
           | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};*/
        blendAttachmentStates.push_back(blendAttachmentState);
    }
    colorBlendState.pAttachments    = blendAttachmentStates.data();
    colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());

    vk::GraphicsPipelineCreateInfo pipelineCreateInfo(
        vk::PipelineCreateFlags(), shaderStages.size(), shaderStages.data(), &vi, &inputAssemblyState,
        &tessellationState, &viewportState, &rasterizationState, &multisampleState, &depthStencilState,
        &colorBlendState, &dynamicState, pipelineLayout, renderPass, 0, vk::Pipeline(), 0);
    return pipelineCreateInfo;
}



}  // namespace Vulkan
}  // namespace Saiga
