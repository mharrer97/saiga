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
#include "saiga/vulkan/lighting/SpotLight.h"


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


SpotLight::SpotLight() {}


void SpotLight::createShadowMap(VulkanBase& vulkanDevice, int w, int h,
                                vk::RenderPass shadowPass)  //, ShadowQuality quality)
{
    shadowMapInitialized = true;
    shadowmap            = std::make_shared<SimpleShadowmap>();  //, quality);
    shadowmap->init(vulkanDevice, w, h, shadowPass);
}

void SpotLight::destroyShadowMap()
{
    shadowMapInitialized = false;
    shadowmap->~SimpleShadowmap();
}


SpotLight& SpotLight::operator=(const SpotLight& light)
{
    model         = light.model;
    colorDiffuse  = light.colorDiffuse;
    colorSpecular = light.colorSpecular;
    attenuation   = light.attenuation;
    cutoffRadius  = light.cutoffRadius;
    direction     = light.direction;
    openingAngle  = light.openingAngle;
    return *this;
}

void SpotLight::recalculateScale()
{
    if (openingAngle < 135.f)
    {
        float l = tan(radians(openingAngle / 2.f)) * cutoffRadius;
        vec3 scale(l, cutoffRadius, l);
        this->setScale(scale);  // make_vec3(cutoffRadius));
    }
    else
    {
        this->setScale(make_vec3(cutoffRadius));
    }
}

float SpotLight::getRadius() const
{
    return cutoffRadius;
}


void SpotLight::setRadius(float value)
{
    cutoffRadius = value;
    recalculateScale();
}

void SpotLight::setAngle(float value)
{
    openingAngle = clamp(value, 0.f, 360.f);
    recalculateScale();
}

vec3 SpotLight::getDirection() const
{
    return direction;
}

void SpotLight::setDirection(vec3 value)
{
    direction = normalize(value);
    rot       = rotation(vec3(0, -1, 0), normalize(direction));
}

void SpotLight::calculateCamera()
{
    vec3 dir = make_vec3(this->getUpVector());
    vec3 pos = vec3(getPosition());
    vec3 up  = make_vec3(getRightVector());
    shadowCamera.setView(pos, pos - dir, up);
    shadowCamera.setProj(std::min(135.f, openingAngle), 1, shadowNearPlane, cutoffRadius, true);
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

bool SpotLight::cullLight(Camera* cam)
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
void SpotLightRenderer::destroy()
{
    Pipeline::destroy();
    uniformBufferVS.destroy();
    uniformBufferFS.destroy();
}

void SpotLightRenderer::render(vk::CommandBuffer cmd, std::shared_ptr<SpotLight> light)
{
    // vec4 pos = vec4(light->position[0], light->position[1], light->position[2], 1.f);
    // updateUniformBuffers(cmd, proj, view, pos, 25.f, false);
    bindDescriptorSet(cmd, descriptorSet);
    // vk::Viewport vp(position[0], position[1], size[0], size[1]);
    // cmd.setViewport(0, vp);
    if (pushConstantObject.openingAngle < 135.f)  // render pyramid if small angle
    {
        lightMesh.render(cmd);
    }
    else  // render icosphere if large angle
    {
        lightMeshIco.render(cmd);
    }
}



void SpotLightRenderer::init(VulkanBase& vulkanDevice, VkRenderPass renderPass, std::string fragmentShader)
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
    info.rasterizationState.cullMode              = vk::CullModeFlagBits::eFront;
    info.depthStencilState.depthWriteEnable       = VK_FALSE;

    // info.blendAttachmentState.
    // info.depthStencilState.depthWriteEnable = VK_TRUE;
    // info.depthStencilState.depthTestEnable  = VK_FALSE;

    create(renderPass, info);


    Cone c(make_vec3(0), vec3(0, 1, 0), 1.0f, 1.0f);
    lightMesh.mesh = *TriangleMeshGenerator::createConeMesh(c, 10);
    //    cb->createBuffers(spotLightMesh);
    // lightMesh.createUniformPyramid();
    lightMesh.init(vulkanDevice);

    lightMeshIco.loadObj("icosphere.obj");
    lightMeshIco.init(vulkanDevice);
}

void SpotLightRenderer::updateUniformBuffers(vk::CommandBuffer cmd, mat4 proj, mat4 view)
{
    uboFS.proj = proj;
    uboFS.view = view;
    uniformBufferFS.update(cmd, sizeof(uboFS), &uboFS);

    uboVS.proj = proj;
    uboVS.view = view;
    uniformBufferVS.update(cmd, sizeof(uboVS), &uboVS);
}

void SpotLightRenderer::createAndUpdateDescriptorSet(Saiga::Vulkan::Memory::ImageMemoryLocation* diffuse,
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

void SpotLightRenderer::pushLight(vk::CommandBuffer cmd, std::shared_ptr<SpotLight> light)
{
    pushConstantObject.attenuation  = make_vec4(light->getAttenuation(), light->getRadius());
    pushConstantObject.pos          = make_vec4(light->getPosition(), 1.f);
    pushConstantObject.dir          = make_vec4(light->getDirection(), 0.f);
    pushConstantObject.openingAngle = light->getAngle();
    pushConstantObject.model        = light->model;
    pushConstantObject.specularCol  = make_vec4(light->getColorSpecular(), 1.f);
    pushConstantObject.diffuseCol   = make_vec4(light->getColorDiffuse(), light->getIntensity());

    // pushConstant(cmd, vk::ShaderStageFlagBits::eVertex, sizeof(mat4), data(translate(light->position)));
    pushConstant(cmd, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, sizeof(pushConstantObject),
                 &pushConstantObject, 0);
}

// Shadow Renderer
void SpotShadowLightRenderer::destroy()
{
    Pipeline::destroy();
    uniformBufferVS.destroy();
    uniformBufferFS.destroy();
}

void SpotShadowLightRenderer::render(vk::CommandBuffer cmd, std::shared_ptr<SpotLight> light,
                                     DescriptorSet& descriptorSet)
{
    // vec4 pos = vec4(light->position[0], light->position[1], light->position[2], 1.f);
    // updateUniformBuffers(cmd, proj, view, pos, 25.f, false);
    bindDescriptorSet(cmd, descriptorSet);
    // vk::Viewport vp(position[0], position[1], size[0], size[1]);
    // cmd.setViewport(0, vp);
    if (pushConstantObject.openingAngle < 135.f)  // render pyramid if small angle
    {
        lightMesh.render(cmd);
    }
    else  // render icosphere if large angle
    {
        lightMeshIco.render(cmd);
    }
}



void SpotShadowLightRenderer::init(VulkanBase& vulkanDevice, VkRenderPass renderPass, std::string fragmentShader)
{
    PipelineBase::init(vulkanDevice, 1);
    addDescriptorSetLayout({{0, {11, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {1, {12, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {2, {13, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {3, {14, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {4, {15, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {5, {16, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {6, {17, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment}},
                            {7, {7, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex}}});

    // addPushConstantRange({vk::ShaderStageFlagBits::eVertex, 0, sizeof(mat4)});
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
    info.rasterizationState.cullMode              = vk::CullModeFlagBits::eFront;
    info.depthStencilState.depthWriteEnable       = VK_FALSE;

    // info.blendAttachmentState.
    // info.depthStencilState.depthWriteEnable = VK_TRUE;
    // info.depthStencilState.depthTestEnable  = VK_FALSE;

    create(renderPass, info);


    Cone c(make_vec3(0), vec3(0, 1, 0), 1.0f, 1.0f);
    lightMesh.mesh = *TriangleMeshGenerator::createConeMesh(c, 10);
    //    cb->createBuffers(spotLightMesh);
    // lightMesh.createUniformPyramid();
    lightMesh.init(vulkanDevice);

    lightMeshIco.loadObj("icosphere.obj");
    lightMeshIco.init(vulkanDevice);

    uniformBufferVS.init(*base, &uboVS, sizeof(uboVS));
    uniformBufferFS.init(*base, &uboFS, sizeof(uboFS));
}

void SpotShadowLightRenderer::updateUniformBuffers(vk::CommandBuffer cmd, mat4 proj, mat4 view)
{
    uboFS.proj = proj;
    uboFS.view = view;
    uniformBufferFS.update(cmd, sizeof(uboFS), &uboFS);

    uboVS.proj = proj;
    uboVS.view = view;
    uniformBufferVS.update(cmd, sizeof(uboVS), &uboVS);
}

void SpotShadowLightRenderer::createAndUpdateDescriptorSet(Saiga::Vulkan::Memory::ImageMemoryLocation* diffuse,
                                                           Saiga::Vulkan::Memory::ImageMemoryLocation* specular,
                                                           Saiga::Vulkan::Memory::ImageMemoryLocation* normal,
                                                           Saiga::Vulkan::Memory::ImageMemoryLocation* additional,
                                                           Saiga::Vulkan::Memory::ImageMemoryLocation* depth,
                                                           Saiga::Vulkan::Memory::ImageMemoryLocation* shadowmap)
{
    auto descriptorSet = createDescriptorSet();


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

    vk::DescriptorImageInfo shadowDescriptorInfo = shadowmap->data.get_descriptor_info();
    shadowDescriptorInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    shadowDescriptorInfo.setImageView(shadowmap->data.view);
    shadowDescriptorInfo.setSampler(shadowmap->data.sampler);



    vk::DescriptorBufferInfo uboVSDescriptorInfo = uniformBufferVS.getDescriptorInfo();

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
            vk::WriteDescriptorSet(descriptorSet, 16, 0, 1, vk::DescriptorType::eCombinedImageSampler,
                                   &shadowDescriptorInfo, nullptr, nullptr),
            vk::WriteDescriptorSet(descriptorSet, 17, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr,
                                   &uboFSDescriptorInfo, nullptr),
            vk::WriteDescriptorSet(descriptorSet, 7, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr,
                                   &uboVSDescriptorInfo, nullptr),
        },
        nullptr);
}

void SpotShadowLightRenderer::updateImageMemoryLocations(Memory::ImageMemoryLocation* diffuse,
                                                         Memory::ImageMemoryLocation* specular,
                                                         Memory::ImageMemoryLocation* normal,
                                                         Memory::ImageMemoryLocation* additional,
                                                         Memory::ImageMemoryLocation* depth)
{
    diffuseLocation    = diffuse;
    specularLocation   = specular;
    normalLocation     = normal;
    additionalLocation = additional;
    depthLocation      = depth;
}

StaticDescriptorSet SpotShadowLightRenderer::createAndUpdateDescriptorSetShadow(Memory::ImageMemoryLocation* shadowmap)
{
    auto descriptorSet = createDescriptorSet();


    vk::DescriptorImageInfo diffuseDescriptorInfo = diffuseLocation->data.get_descriptor_info();
    diffuseDescriptorInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    diffuseDescriptorInfo.setImageView(diffuseLocation->data.view);
    diffuseDescriptorInfo.setSampler(diffuseLocation->data.sampler);

    vk::DescriptorImageInfo specularDescriptorInfo = specularLocation->data.get_descriptor_info();
    specularDescriptorInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    specularDescriptorInfo.setImageView(specularLocation->data.view);
    specularDescriptorInfo.setSampler(specularLocation->data.sampler);

    vk::DescriptorImageInfo normalDescriptorInfo = normalLocation->data.get_descriptor_info();
    normalDescriptorInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    normalDescriptorInfo.setImageView(normalLocation->data.view);
    normalDescriptorInfo.setSampler(normalLocation->data.sampler);

    vk::DescriptorImageInfo additionalDescriptorInfo = additionalLocation->data.get_descriptor_info();
    additionalDescriptorInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    additionalDescriptorInfo.setImageView(additionalLocation->data.view);
    additionalDescriptorInfo.setSampler(additionalLocation->data.sampler);

    vk::DescriptorImageInfo depthDescriptorInfo = depthLocation->data.get_descriptor_info();
    depthDescriptorInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    depthDescriptorInfo.setImageView(depthLocation->data.view);
    depthDescriptorInfo.setSampler(depthLocation->data.sampler);

    vk::DescriptorImageInfo shadowDescriptorInfo = shadowmap->data.get_descriptor_info();
    shadowDescriptorInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    shadowDescriptorInfo.setImageView(shadowmap->data.view);
    shadowDescriptorInfo.setSampler(shadowmap->data.sampler);



    vk::DescriptorBufferInfo uboVSDescriptorInfo = uniformBufferVS.getDescriptorInfo();

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
            vk::WriteDescriptorSet(descriptorSet, 16, 0, 1, vk::DescriptorType::eCombinedImageSampler,
                                   &shadowDescriptorInfo, nullptr, nullptr),
            vk::WriteDescriptorSet(descriptorSet, 17, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr,
                                   &uboFSDescriptorInfo, nullptr),
            vk::WriteDescriptorSet(descriptorSet, 7, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr,
                                   &uboVSDescriptorInfo, nullptr),
        },
        nullptr);
    return descriptorSet;
}

void SpotShadowLightRenderer::pushLight(vk::CommandBuffer cmd, std::shared_ptr<SpotLight> light, Camera* cam)
{
    pushConstantObject.depthBiasMV = light->viewToLightTransform(*cam, light->shadowCamera);

    pushConstantObject.attenuation  = make_vec4(light->getAttenuation(), light->getRadius());
    pushConstantObject.pos          = make_vec4(light->getPosition(), 1.f);
    pushConstantObject.dir          = make_vec4(light->getDirection(), 0.f);
    pushConstantObject.openingAngle = light->getAngle();
    pushConstantObject.model        = light->model;
    pushConstantObject.specularCol  = make_vec4(light->getColorSpecular(), 1.f);
    pushConstantObject.diffuseCol   = make_vec4(light->getColorDiffuse(), light->getIntensity());

    // pushConstant(cmd, vk::ShaderStageFlagBits::eVertex, sizeof(mat4), data(translate(light->position)));
    pushConstant(cmd, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, sizeof(pushConstantObject),
                 &pushConstantObject, 0);
}
}  // namespace Lighting
}  // namespace Vulkan
}  // namespace Saiga
