/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 *
 * Created by Mathias Harrer: mathias.mh.harrer@fau.de
 */


/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#include "saiga/core/imgui/imgui.h"
#include "saiga/core/util/assert.h"
//#include "saiga/opengl/error.h"
//#include "saiga/opengl/rendering/deferredRendering/deferredRendering.h"
#include "saiga/core/geometry/cone.h"
#include "saiga/core/geometry/triangle_mesh_generator.h"
#include "saiga/core/math/all.h"
#include "saiga/vulkan/lighting/DebugLight.h"


namespace Saiga
{
namespace Vulkan
{
namespace Lighting
{
// Renderer
void DebugLightRenderer::destroy()
{
    Pipeline::destroy();
    uniformBufferVS.destroy();
    uniformBufferFS.destroy();
}

void DebugLightRenderer::render(vk::CommandBuffer cmd, std::shared_ptr<SpotLight> light)
{
    // vec4 pos = vec4(light->position[0], light->position[1], light->position[2], 1.f);
    // updateUniformBuffers(cmd, proj, view, pos, 25.f, false);
    bindDescriptorSet(cmd, descriptorSet);
    // vk::Viewport vp(position[0], position[1], size[0], size[1]);
    // cmd.setViewport(0, vp);
    if (light->getAngle() < 135.f)  // render pyramid if small angle
    {
        lightMeshSpot.render(cmd);
    }
    else  // render icosphere if large angle
    {
        lightMeshPoint.render(cmd);
    }
}

void DebugLightRenderer::render(vk::CommandBuffer cmd, std::shared_ptr<PointLight> light)
{
    bindDescriptorSet(cmd, descriptorSet);
    lightMeshPoint.render(cmd);
}

void DebugLightRenderer::render(vk::CommandBuffer cmd, std::shared_ptr<BoxLight> light)
{
    bindDescriptorSet(cmd, descriptorSet);
    lightMeshBox.render(cmd);
}


void DebugLightRenderer::init(VulkanBase& vulkanDevice, VkRenderPass renderPass, std::string fragmentShader,
                              float lineWidth)
{
    PipelineBase::init(vulkanDevice, 1);
    addDescriptorSetLayout({{0, {11, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {1, {12, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {2, {13, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {3, {14, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {4, {15, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {5, {16, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment}},
                            {6, {7, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex}}});

    // addPushConstantRange({vk::ShaderStageFlagBits::eVertex, 0, sizeof(mat4)});
    addPushConstantRange({vk::ShaderStageFlagBits::eAllGraphics, 0, sizeof(pushConstantObject)});
    shaderPipeline.load(device, {vertexShader, "vulkan/lighting/debugLight.frag"});
    PipelineInfo info;
    info.addVertexInfo<VertexType>();


    info.inputAssemblyState.topology = vk::PrimitiveTopology::eLineList;
    if (base->enabledFeatures.wideLines)
    {
        info.rasterizationState.lineWidth = lineWidth;
    }
    else
    {
        if (lineWidth != 1.0f)
        {
            LOG(WARNING) << "Line width " << lineWidth << " requested, wide lines is not enabled or supported";
        }
    }

    /*info.blendAttachmentState.blendEnable         = VK_TRUE;
    info.blendAttachmentState.alphaBlendOp        = vk::BlendOp::eAdd;
    info.blendAttachmentState.colorBlendOp        = vk::BlendOp::eAdd;
    info.blendAttachmentState.dstColorBlendFactor = vk::BlendFactor::eOne;
    info.blendAttachmentState.srcColorBlendFactor = vk::BlendFactor::eOne;
    info.blendAttachmentState.dstAlphaBlendFactor = vk::BlendFactor::eOne;
    info.blendAttachmentState.srcAlphaBlendFactor = vk::BlendFactor::eOne;
*/
    info.rasterizationState.cullMode        = vk::CullModeFlagBits::eNone;
    info.depthStencilState.depthWriteEnable = VK_FALSE;

    // info.blendAttachmentState.
    // info.depthStencilState.depthWriteEnable = VK_TRUE;
    // info.depthStencilState.depthTestEnable  = VK_FALSE;

    create(renderPass, info);


    Cone c(make_vec3(0), vec3(0, 1, 0), 1.0f, 1.0f);
    lightMeshSpot.mesh = *TriangleMeshGenerator::createConeMesh(c, 10);
    //    cb->createBuffers(spotLightMesh);
    // lightMesh.createUniformPyramid();
    lightMeshSpot.init(vulkanDevice);

    lightMeshPoint.loadObj("icosphere.obj");
    lightMeshPoint.init(vulkanDevice);

    lightMeshBox.loadObj("box.obj");
    lightMeshBox.init(vulkanDevice);
}

void DebugLightRenderer::updateUniformBuffers(vk::CommandBuffer cmd, mat4 proj, mat4 view)
{
    uboFS.proj = proj;
    uboFS.view = view;
    uniformBufferFS.update(cmd, sizeof(uboFS), &uboFS);

    uboVS.proj = proj;
    uboVS.view = view;
    uniformBufferVS.update(cmd, sizeof(uboVS), &uboVS);
}

void DebugLightRenderer::createAndUpdateDescriptorSet(Saiga::Vulkan::Memory::ImageMemoryLocation* diffuse,
                                                      Saiga::Vulkan::Memory::ImageMemoryLocation* specular,
                                                      Saiga::Vulkan::Memory::ImageMemoryLocation* normal,
                                                      Saiga::Vulkan::Memory::ImageMemoryLocation* additional,
                                                      Saiga::Vulkan::Memory::ImageMemoryLocation* depth)
{
    descriptorSet = createDescriptorSet();


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

    uniformBufferVS.init(*base, &uboVS, sizeof(uboVS));
    vk::DescriptorBufferInfo uboVSDescriptorInfo = uniformBufferVS.getDescriptorInfo();

    uniformBufferFS.init(*base, &uboFS, sizeof(uboFS));
    vk::DescriptorBufferInfo uboFSDescriptorInfo = uniformBufferFS.getDescriptorInfo();

    device.updateDescriptorSets(
        {
            vk::WriteDescriptorSet(descriptorSet, 11, 0, 1, vk::DescriptorType::eCombinedImageSampler,
                                   &diffuseDescriptorInfo, nullptr, nullptr),
            vk::WriteDescriptorSet(descriptorSet, 12, 0, 1, vk::DescriptorType::eCombinedImageSampler,
                                   &specularDescriptorInfo, nullptr, nullptr),
            vk::WriteDescriptorSet(descriptorSet, 13, 0, 1, vk::DescriptorType::eCombinedImageSampler,
                                   &normalDescriptorInfo, nullptr, nullptr),
            vk::WriteDescriptorSet(descriptorSet, 14, 0, 1, vk::DescriptorType::eCombinedImageSampler,
                                   &additionalDescriptorInfo, nullptr, nullptr),
            vk::WriteDescriptorSet(descriptorSet, 15, 0, 1, vk::DescriptorType::eCombinedImageSampler,
                                   &depthDescriptorInfo, nullptr, nullptr),
            vk::WriteDescriptorSet(descriptorSet, 16, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr,
                                   &uboFSDescriptorInfo, nullptr),
            vk::WriteDescriptorSet(descriptorSet, 7, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr,
                                   &uboVSDescriptorInfo, nullptr),
        },
        nullptr);
}

void DebugLightRenderer::pushLight(vk::CommandBuffer cmd, std::shared_ptr<Light> light)
{
    pushConstantObject.model = light->model;


    // pushConstant(cmd, vk::ShaderStageFlagBits::eVertex, sizeof(mat4), data(translate(light->position)));
    pushConstant(cmd, vk::ShaderStageFlagBits::eAllGraphics, sizeof(pushConstantObject), &pushConstantObject, 0);
}



}  // namespace Lighting
}  // namespace Vulkan
}  // namespace Saiga
