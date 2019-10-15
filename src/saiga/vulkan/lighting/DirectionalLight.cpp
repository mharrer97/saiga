/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 *
 * Created by Mathias Harrer: mathias.mh.harrer@fau.de
 */

#include "saiga/vulkan/lighting/DirectionalLight.h"

/*#include "saiga/core/geometry/aabb.h"
#include "saiga/core/geometry/triangle_mesh_generator.h"
#include "saiga/core/imgui/imgui.h"
#include "saiga/core/util/assert.h"
#include "saiga/vulkan/Vertex.h"
*/
namespace Saiga
{
namespace Vulkan
{
namespace Lighting
{
// light
DirectionalLight::DirectionalLight() {}

void DirectionalLight::setView(vec3 pos, vec3 target, vec3 up)
{
    //    this->setViewMatrix(lookAt(pos,pos + (pos-target),up));
    this->setViewMatrix(lookAt(pos, target, up));
}

void DirectionalLight::setDirection(vec3 dir)
{
    this->setView(-dir, make_vec3(0.f), vec3(0.f, 1.f, 0.f));
}

void DirectionalLight::calculateCamera()
{
    // the camera is centred at the centre of the shadow volume.
    // we define the box only by the sides of the orthographic projection
    calculateModel();
    // trs matrix without scale
    //(scale is applied through projection matrix
    mat4 T = translate(identityMat4(), make_vec3(position));
    mat4 R = make_mat4(rot);
    mat4 m = T * R;
    shadowCamera.setView(inverse(m));
    //    shadowCamera.setProj(-scale[0], scale[0], -scale[1], scale[1], -scale[2], scale[2]);
    shadowCamera.setProj(-scale[0], scale[0], -scale[1], scale[1], -scale[2], scale[2]);
}

bool DirectionalLight::cullLight(Camera* cam)
{
    /*Sphere s(getPosition(), cutoffRadius);
    this->culled = cam->sphereInFrustum(s) == Camera::OUTSIDE;*/
    this->culled = false;
    //    this->culled = false;
    //    std::cout<<culled<<endl;
    return culled;
}


// renderer
void DirectionalLightRenderer::destroy()
{
    Pipeline::destroy();
    uniformBufferFS.destroy();
}

void DirectionalLightRenderer::render(vk::CommandBuffer cmd, std::shared_ptr<DirectionalLight> light)
{
    bindDescriptorSet(cmd, descriptorSet);
    // vk::Viewport vp(position[0], position[1], size[0], size[1]);
    // cmd.setViewport(0, vp);
    blitMesh.render(cmd);
}

void DirectionalLightRenderer::init(VulkanBase& vulkanDevice, VkRenderPass renderPass, std::string fragmentShader)
{
    PipelineBase::init(vulkanDevice, 1);
    addDescriptorSetLayout({{0, {11, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {1, {12, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {2, {13, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {3, {14, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {4, {15, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {5, {16, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment}}});

    addPushConstantRange(
        {vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(pushConstantObject)});
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

void DirectionalLightRenderer::updateUniformBuffers(vk::CommandBuffer cmd, mat4 proj, mat4 view, bool debug)
{
    uboFS.proj  = proj;
    uboFS.view  = view;
    uboFS.debug = debug;
    uniformBufferFS.update(cmd, sizeof(uboFS), &uboFS);
}
void DirectionalLightRenderer::createAndUpdateDescriptorSet(Saiga::Vulkan::Memory::ImageMemoryLocation* diffuse,
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

        },
        nullptr);
}
void DirectionalLightRenderer::pushLight(vk::CommandBuffer cmd, std::shared_ptr<DirectionalLight> light)
{
    pushConstantObject.model       = light->model;
    pushConstantObject.specularCol = make_vec4(light->getColorSpecular(), 1.f);
    pushConstantObject.diffuseCol  = make_vec4(light->getColorDiffuse(), light->getIntensity());
    // pushConstant(cmd, vk::ShaderStageFlagBits::eVertex, sizeof(mat4), data(translate(light->position)));
    pushConstant(cmd, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, sizeof(pushConstantObject),
                 &pushConstantObject, 0);
}

}  // namespace Lighting

}  // namespace Vulkan
}  // namespace Saiga
