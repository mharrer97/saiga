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
#include "saiga/vulkan/lighting/PointLight.h"

namespace Saiga
{
namespace Vulkan
{
namespace Lighting
{
/*void PointLightShader::checkUniforms()
{
    AttenuatedLightShader::checkUniforms();
    location_shadowPlanes = getUniformLocation("shadowPlanes");
}



void PointLightShader::uploadShadowPlanes(float f, float n)
{
    Shader::upload(location_shadowPlanes, vec2(f, n));
}*/


PointLight::PointLight() {}


PointLight& PointLight::operator=(const PointLight& light)
{
    model         = light.model;
    colorDiffuse  = light.colorDiffuse;
    colorSpecular = light.colorSpecular;
    attenuation   = light.attenuation;
    cutoffRadius  = light.cutoffRadius;
    return *this;
}


float PointLight::getRadius() const
{
    return cutoffRadius;
}


void PointLight::setRadius(float value)
{
    cutoffRadius = value;
    this->setScale(make_vec3(cutoffRadius));
}

/*void PointLight::bindUniforms(std::shared_ptr<PointLightShader> shader, Camera* cam)
{
    AttenuatedLight::bindUniforms(shader, cam);
    shader->uploadShadowPlanes(this->shadowCamera.zFar, this->shadowCamera.zNear);
    shader->uploadInvProj(inverse(cam->proj));
    if (this->hasShadows())
    {
        shader->uploadDepthBiasMV(viewToLightTransform(*cam, this->shadowCamera));
        shader->uploadDepthTexture(shadowmap->getDepthTexture());
        shader->uploadShadowMapSize(shadowmap->getSize());
    }
    assert_no_glerror();
}*/



/*void PointLight::createShadowMap(int w, int h, ShadowQuality quality)
{
    shadowmap = std::make_shared<CubeShadowmap>(w, h, quality);
    //    shadowmap->createCube(w,h);
}*/



/*struct CameraDirection
{
    GLenum CubemapFace;
    vec3 Target;
    vec3 Up;
};

static const CameraDirection gCameraDirections[] = {
    {GL_TEXTURE_CUBE_MAP_POSITIVE_X, vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, -1.0f, 0.0f)},
    {GL_TEXTURE_CUBE_MAP_NEGATIVE_X, vec3(-1.0f, 0.0f, 0.0f), vec3(0.0f, -1.0f, 0.0f)},
    {GL_TEXTURE_CUBE_MAP_POSITIVE_Y, vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f)},
    {GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, vec3(0.0f, -1.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f)},
    {GL_TEXTURE_CUBE_MAP_POSITIVE_Z, vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, -1.0f, 0.0f)},
    {GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, -1.0f, 0.0f)}};


void PointLight::bindFace(int face)
{
    shadowmap->bindCubeFace(gCameraDirections[face].CubemapFace);
}
*/

/*void PointLight::calculateCamera(int face)
{
    vec3 pos(this->getPosition());
    vec3 dir(gCameraDirections[face].Target);
    vec3 up(gCameraDirections[face].Up);
    shadowCamera.setView(pos, pos + dir, up);
    shadowCamera.setProj(90.0f, 1, shadowNearPlane, cutoffRadius);
}*/

bool PointLight::cullLight(Camera* cam)
{
    Sphere s(getPosition(), cutoffRadius);
    this->culled = cam->sphereInFrustum(s) == Camera::OUTSIDE;
    //    this->culled = false;
    //    std::cout<<culled<<endl;
    return culled;
}

/*
bool PointLight::renderShadowmap(DepthFunction f, UniformBuffer& shadowCameraBuffer)
{
    if (shouldCalculateShadowMap())
    {
        for (int i = 0; i < 6; i++)
        {
            bindFace(i);
            calculateCamera(i);
            shadowCamera.recalculatePlanes();
            CameraDataGLSL cd(&shadowCamera);
            shadowCameraBuffer.updateBuffer(&cd, sizeof(CameraDataGLSL), 0);
            f(&shadowCamera);
            shadowmap->unbindFramebuffer();
        }
        return true;
    }
    else
    {
        return false;
    }
}

void PointLight::renderImGui()
{
    AttenuatedLight::renderImGui();
    ImGui::InputFloat("shadowNearPlane", &shadowNearPlane);
}*/



// Renderer
void PointLightRenderer::destroy()
{
    Pipeline::destroy();
    uniformBufferVS.destroy();
    uniformBufferFS.destroy();
}

void PointLightRenderer::render(vk::CommandBuffer cmd, std::shared_ptr<PointLight> light)
{
    // vec4 pos = vec4(light->position[0], light->position[1], light->position[2], 1.f);
    // updateUniformBuffers(cmd, proj, view, pos, 25.f, false);
    bindDescriptorSet(cmd, descriptorSet);
    // vk::Viewport vp(position[0], position[1], size[0], size[1]);
    // cmd.setViewport(0, vp);
    lightMesh.render(cmd);
}



void PointLightRenderer::init(VulkanBase& vulkanDevice, VkRenderPass renderPass)
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
    addPushConstantRange({vk::ShaderStageFlagBits::eFragment, 0, sizeof(pushConstantObject)});
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

    lightMesh.loadObj("box.obj");
    lightMesh.init(vulkanDevice);
}

void PointLightRenderer::updateUniformBuffers(vk::CommandBuffer cmd, mat4 proj, mat4 view, bool debug)
{
    uboFS.proj  = proj;
    uboFS.view  = view;
    uboFS.debug = debug;
    uniformBufferFS.update(cmd, sizeof(uboFS), &uboFS);

    uboVS.proj = proj;
    uboVS.view = view;
    uniformBufferVS.update(cmd, sizeof(uboVS), &uboVS);
}

void PointLightRenderer::createAndUpdateDescriptorSet(Saiga::Vulkan::Memory::ImageMemoryLocation* diffuse,
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

void PointLightRenderer::pushLight(vk::CommandBuffer cmd, std::shared_ptr<PointLight> light)
{
    pushConstantObject.attenuation = make_vec4(light->getAttenuation(), light->getRadius());
    pushConstantObject.pos         = make_vec4(light->position, 1.f);

    pushConstant(cmd, vk::ShaderStageFlagBits::eFragment, sizeof(pushConstantObject), &pushConstantObject, 0);
    // pushConstant(cmd, vk::ShaderStageFlagBits::eVertex, sizeof(mat4), data(model));
}


}  // namespace Lighting
}  // namespace Vulkan
}  // namespace Saiga



namespace Saiga
{
namespace Vulkan
{
namespace Lighting
{
OldPointLight::OldPointLight(vec3 pos, vec3 dir, float angle) : light_pos(pos)
{
    this->light_dir = normalize(dir);
    this->angle     = clamp(angle, 0.f, 180.f);
}

//!
//! \brief PointLight::setPosition sets the position of the pointlight
//! \param pos
//!
void OldPointLight::setPosition(vec3 pos)
{
    light_pos = pos;
}

//!
//! \brief PointLight::setDirection sets the lights direction to the normalized incoming parameter
//! \param dir
//! \return normalized direction
//!
vec3 OldPointLight::setDirection(vec3 dir)
{
    light_dir = normalize(dir);
    return light_dir;
}

//!
//! \brief PointLight::setAngle sets lights opening angle to the given angle alamped to 0 to 180 degrees
//! \param angle
//! \return the clamped angle
//!
float OldPointLight::setAngle(float angle)
{
    this->angle = clamp(angle, 0.f, 180.f);
    return this->angle;
}

//!
//! \brief PointLight::getPosition
//! \return position of the light
//!
vec3 OldPointLight::getPosition()
{
    return light_pos;
}

//!
//! \brief PointLight::getDirection
//! \return direction of the light
//!
vec3 OldPointLight::getDirection()
{
    return light_dir;
}

//!
//! \brief PointLight::getAngle
//! \return opening angle of the light
//!
float OldPointLight::getAngle()
{
    return angle;
}
}  // namespace Lighting
}  // namespace Vulkan
}  // namespace Saiga
