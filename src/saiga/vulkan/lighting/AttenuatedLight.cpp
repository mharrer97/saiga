/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 *
 * Created by Mathias Harrer: mathias.mh.harrer@fau.de
 *
 */

#include "saiga/vulkan/lighting/AttenuatedLight.h"

#include "saiga/core/imgui/imgui.h"
#include "saiga/core/util/assert.h"
//#include "saiga/opengl/error.h"
namespace Saiga
{
namespace Vulkan
{
namespace Lighting
{
/*void AttenuatedLightShader::checkUniforms()
{
    LightShader::checkUniforms();
    location_attenuation = getUniformLocation("attenuation");
}


void AttenuatedLightShader::uploadA(vec3& attenuation, float cutoffRadius)
{
    Shader::upload(location_attenuation, make_vec4(attenuation, cutoffRadius));
}*/

AttenuatedLight::AttenuatedLight() {}

AttenuatedLight& AttenuatedLight::operator=(const AttenuatedLight& light)
{
    model         = light.model;
    colorDiffuse  = light.colorDiffuse;
    colorSpecular = light.colorSpecular;
    attenuation   = light.attenuation;
    cutoffRadius  = light.cutoffRadius;
    position      = light.position;
    return *this;
}

float AttenuatedLight::evaluateAttenuation(float distance)
{
    // normalize the distance, so the attenuation is independent of the radius
    float x = distance / cutoffRadius;

    return 1.0f / (attenuation[0] + attenuation[1] * x + attenuation[2] * x * x);
}

vec3 AttenuatedLight::getAttenuation() const
{
    return attenuation;
}

float AttenuatedLight::getAttenuation(float r)
{
    float x = r / cutoffRadius;
    return 1.0 / (attenuation[0] + attenuation[1] * x + attenuation[2] * x * x);
}

void AttenuatedLight::setAttenuation(const vec3& value)
{
    attenuation = value;
}


float AttenuatedLight::getRadius() const
{
    return cutoffRadius;
}


void AttenuatedLight::setRadius(float value)
{
    cutoffRadius = value;
    this->setScale(make_vec3(cutoffRadius));
}

/*void AttenuatedLight::bindUniforms(std::shared_ptr<AttenuatedLightShader> shader, Camera* cam)
{
    if (isVolumetric()) shader->uploadVolumetricDensity(volumetricDensity);
    shader->uploadColorDiffuse(colorDiffuse);
    shader->uploadColorSpecular(colorSpecular);
    shader->uploadModel(model);
    shader->uploadA(attenuation, cutoffRadius);
    assert_no_glerror();
}


void AttenuatedLight::renderImGui()
{
    Light::renderImGui();
    ImGui::InputFloat3("attenuation", &attenuation[0]);
    float c = evaluateAttenuation(cutoffRadius);
    ImGui::Text("Cutoff Intensity: %f", c);
    ImGui::InputFloat("cutoffRadius", &cutoffRadius);
}
*/



// Renderer
void AttenuatedLightRenderer::destroy()
{
    Pipeline::destroy();
    uniformBufferVS.destroy();
}

void AttenuatedLightRenderer::render(vk::CommandBuffer cmd, std::shared_ptr<AttenuatedLight> light)
{
    // vec4 pos = vec4(light->position[0], light->position[1], light->position[2], 1.f);
    // updateUniformBuffers(cmd, proj, view, pos, 25.f, false);
    bindDescriptorSet(cmd, descriptorSet);
    // vk::Viewport vp(position[0], position[1], size[0], size[1]);
    // cmd.setViewport(0, vp);
    lightMesh.render(cmd);
}



void AttenuatedLightRenderer::init(VulkanBase& vulkanDevice, VkRenderPass renderPass, std::string fragmentShader)
{
    PipelineBase::init(vulkanDevice, 1);
    addDescriptorSetLayout({{0, {11, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {1, {12, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {2, {13, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {3, {14, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {4, {15, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {5, {16, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment}}});

    // addPushConstantRange({vk::ShaderStageFlagBits::eVertex, 0, sizeof(mat4)});
    addPushConstantRange(
        {vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(pushConstantObject)});
    shaderPipeline.load(device, {vertexShader, fragmentShader});
    PipelineInfo info;
    info.addVertexInfo<VertexType>();
    info.rasterizationState.cullMode              = vk::CullModeFlagBits::eNone;
    info.blendAttachmentState.blendEnable         = VK_TRUE;
    info.blendAttachmentState.alphaBlendOp        = vk::BlendOp::eAdd;
    info.blendAttachmentState.colorBlendOp        = vk::BlendOp::eAdd;
    info.blendAttachmentState.dstColorBlendFactor = vk::BlendFactor::eOne;
    info.blendAttachmentState.srcColorBlendFactor = vk::BlendFactor::eOne;
    info.blendAttachmentState.dstAlphaBlendFactor = vk::BlendFactor::eOne;
    info.blendAttachmentState.srcAlphaBlendFactor = vk::BlendFactor::eOne;

    // info.blendAttachmentState.
    // info.depthStencilState.depthWriteEnable = VK_TRUE;
    // info.depthStencilState.depthTestEnable  = VK_FALSE;

    create(renderPass, info);

    lightMesh.createFullscreenQuad();
    lightMesh.init(vulkanDevice);
}

void AttenuatedLightRenderer::updateUniformBuffers(vk::CommandBuffer cmd, mat4 proj, mat4 view, vec4 lightPosition,
                                                   float intensity, bool debug)
{
    uboVS.proj      = proj;
    uboVS.view      = view;
    uboVS.lightPos  = lightPosition;
    uboVS.debug     = debug;
    uboVS.intensity = intensity;
    uniformBufferVS.update(cmd, sizeof(uboVS), &uboVS);
}

void AttenuatedLightRenderer::createAndUpdateDescriptorSet(Saiga::Vulkan::Memory::ImageMemoryLocation* diffuse,
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
    vk::DescriptorBufferInfo uboDescriptorInfo = uniformBufferVS.getDescriptorInfo();

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
                                   &uboDescriptorInfo, nullptr),
        },
        nullptr);
}

void AttenuatedLightRenderer::pushPosition(vk::CommandBuffer cmd, vec4 pos)
{
    pushConstant(cmd, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, sizeof(pushConstantObject),
                 data(pos), 0);
    // pushConstant(cmd, vk::ShaderStageFlagBits::eVertex, sizeof(mat4), data(model));
}
}  // namespace Lighting
}  // namespace Vulkan
}  // namespace Saiga
