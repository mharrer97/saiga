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


void DirectionalLight::createShadowMap(VulkanBase& vulkanDevice, int w, int h,
                                       vk::RenderPass shadowPass)  //, ShadowQuality quality)
{
    shadowMapInitialized = true;
    shadowmap            = std::make_shared<SimpleShadowmap>();  //, quality);
    shadowmap->init(vulkanDevice, w, h, shadowPass);


    numCascades = 1;
    orthoBoxes.resize(numCascades);


    depthCutsRelative.resize(numCascades + 1);
    depthCuts.resize(numCascades + 1);

    for (int i = 0; i < numCascades; ++i)
    {
        depthCutsRelative[i] = float(i) / numCascades;
    }
    depthCutsRelative.back() = 1.0f;
}

void DirectionalLight::destroyShadowMap()
{
    shadowMapInitialized = false;
    shadowmap->~SimpleShadowmap();
}

void DirectionalLight::setView(vec3 pos, vec3 target, vec3 up)
{
    //    this->setViewMatrix(lookAt(pos,pos + (pos-target),up));
    direction = (target - pos);
    direction = normalize(direction);
    this->setViewMatrix(lookAt(pos, target, up));
}

void DirectionalLight::calculateCamera()
{
    // the camera is centred at the centre of the shadow volume.
    // we define the box only by the sides of the orthographic projection
    calculateModel();
    // trs matrix without scale
    //(scale is applied through projection matrix
    mat4 T = translate(identityMat4(), make_vec3(10));
    mat4 R = make_mat4(rot);
    mat4 m = T * R;
    shadowCamera.setView(inverse(m));
    //    shadowCamera.setProj(-scale[0], scale[0], -scale[1], scale[1], -scale[2], scale[2]);
    shadowCamera.setProj(-scale[0], scale[0], -scale[1], scale[1], -scale[2], scale[2]);
}
void DirectionalLight::setDirection(const vec3& dir)
{
    direction = normalize(dir);

    vec3 d     = -direction;
    vec3 right = normalize(cross(vec3(1, 1, 0), d));
    vec3 up    = normalize(cross(d, right));


    mat3 m;
    col(m, 0) = right;
    col(m, 1) = up;
    col(m, 2) = d;

    vec3 cp = make_vec3(0);

    this->shadowCamera.setPosition(cp);


    this->shadowCamera.rot = quat_cast(m);

    this->shadowCamera.calculateModel();
    this->shadowCamera.updateFromModel();
}

void DirectionalLight::fitShadowToCamera(Camera* cam)
{
#if 0
    vec3 dir = -direction;
    vec3 right = normalize(cross(vec3(1,1,0),dir));
    vec3 up = normalize(cross(dir,right));

    OBB obb;
    obb.setOrientationScale( normalize(right), normalize(up), normalize(dir) );

    obb.fitToPoints(0,cam->vertices,8);
    obb.fitToPoints(1,cam->vertices,8);
    obb.fitToPoints(2,cam->vertices,8);


    vec3 increase(0,0,5.0);

    float xDiff = 2.0f * length(obb.orientationScale[0]) + increase[0];
    float yDiff = 2.0f * length(obb.orientationScale[1]) + increase[1];
    float zDiff = 2.0f * length(obb.orientationScale[2]) + increase[2];

    shadowNearPlane = 0;
    this->cam.setProj(
                -xDiff / 2.0f ,xDiff / 2.0f,
                -yDiff / 2.0f ,yDiff / 2.0f,
                -zDiff / 2.0f ,zDiff / 2.0f
                );

    this->cam.setPosition( obb.center );

    obb.normalize();
    this->cam.rot = quat_cast( obb.orientationScale );

    this->cam.calculateModel();
    this->cam.updateFromModel();


    //    vec4 test = this->cam.proj * this->cam.view * vec4(obb.center,1);
    //    std::cout << "test " << test << std::endl;
#else
    // other idea use bounding sphere of frustum
    // make sure shadow box aligned to light fits bounding sphere
    // note: camera movement or rotation doesn't change the size of the shadow box anymore
    // translate the box only by texel size increments to remove flickering



    Sphere boundingSphere = cam->boundingSphere;

    for (int i = 0; i < (int)depthCutsRelative.size(); ++i)
    {
        float a      = depthCutsRelative[i];
        depthCuts[i] = (1.0f - a) * cam->zNear + (a)*cam->zFar;
    }

    for (int c = 0; c < numCascades; ++c)
    {
        AABB& orthoBox = orthoBoxes[c];

        {
            PerspectiveCamera* pc = static_cast<PerspectiveCamera*>(cam);
            // compute bounding sphere for cascade

            //        vec3 d = -vec3(cam->model[2]);
            vec3 right = make_vec3(col(cam->model, 0));
            vec3 up    = make_vec3(col(cam->model, 1));
            vec3 dir   = -make_vec3(col(cam->model, 2));


            float zNear = depthCuts[c] - cascadeInterpolateRange;
            float zFar  = depthCuts[c + 1] + cascadeInterpolateRange;
            //        float zNear = cam->zNear;
            //        float zFar = cam->zFar;

            //            float zNear = 1;
            //            float zFar = 10;

            //            if(c == 1){
            //                zNear = 10;
            //                zFar = 50;
            //            }

            //            std::cout << "znear/far: " << zNear << " " << zFar << std::endl;

            float tang = (float)tan(pc->fovy * 0.5);

            float fh = zFar * tang;
            float fw = fh * pc->aspect;

            vec3 nearplanepos = cam->getPosition() + dir * zNear;
            vec3 farplanepos  = cam->getPosition() + dir * zFar;
            vec3 v            = farplanepos + fh * up - fw * right;


            vec3 sphereMid = (nearplanepos + farplanepos) * 0.5f;
            float r        = distance(v, sphereMid);

            boundingSphere.r   = r;
            boundingSphere.pos = sphereMid;
        }

        vec3 lightPos = this->shadowCamera.getPosition();

        float r = boundingSphere.r;
        r       = ceil(r);

        vec3 smsize = make_vec3(make_vec2(shadowmap->getSize()), 128468);

        vec3 texelSize;
        //    texelSize[0] = 2.0f * r / shadowmap.w;
        //    texelSize[1] = 2.0f * r / shadowmap.h;
        //    texelSize[2] = 0.0001f;
        texelSize = 2.0f * r / smsize;

        // project the position of the actual camera to light space
        vec3 p = boundingSphere.pos;
        mat3 v = make_mat3(this->shadowCamera.view);
        vec3 t = v * p - v * lightPos;
        t[2]   = -t[2];



        orthoBox.min = t - make_vec3(r);
        orthoBox.max = t + make_vec3(r);

#    if 1
        {
            // move camera in texel size increments
            orthoBox.min = ele_div(orthoBox.min, texelSize);
            orthoBox.min = floor(orthoBox.min);
            orthoBox.min = ele_mult(orthoBox.min, texelSize);

            //            orthoBox.max /= texelSize;
            orthoBox.max = ele_div(orthoBox.max, texelSize);
            orthoBox.max = floor(orthoBox.max);
            orthoBox.max = ele_mult(orthoBox.max, texelSize);
        }
#    endif
    }

    //    this->cam.setProj(orthoBox);
    //    this->cam.setProj(
    //                orthoMin[0] ,orthoMax[0],
    //                orthoMin[1] ,orthoMax[1],
    //                orthoMin[2] ,orthoMax[2]
    //                );

#    ifdef SAIGA_DEBUG1
    // test if all cam vertices are in the shadow volume
    for (int i = 0; i < 8; ++i)
    {
        vec3 v = cam->vertices[i];
        vec4 p = shadowCamera.proj * shadowCamera.view * make_vec4(v, 1);
        std::cout << p << std::endl;
    }
#    endif

#endif
}

/*void DirectionalLight::fitNearPlaneToScene(AABB sceneBB)
{
    //    vec3 orthoMin(cam.left,cam.bottom,cam[2]Near);
    //    vec3 orthoMax(cam.right,cam.top,cam[2]Far);


    for (auto& orthoBox : orthoBoxes)
    {
        // transform scene AABB to light space
        auto tris = sceneBB.toTriangles();
        std::vector<PolygonType> trisp;
        for (auto t : tris)
        {
            trisp.push_back(Polygon::toPolygon(t));
        }
        for (auto& p : trisp)
        {
            for (auto& v : p)
            {
                v = make_vec3(this->shadowCamera.view * make_vec4(v, 1));
            }
        }


        // clip triangles of scene AABB to the 4 side planes of the frustum
        for (auto& p : trisp)
        {
            p = Clipping::clipPolygonAxisAlignedPlane(p, 0, orthoBox.min[0], true);
            p = Clipping::clipPolygonAxisAlignedPlane(p, 0, orthoBox.max[0], false);

            p = Clipping::clipPolygonAxisAlignedPlane(p, 1, orthoBox.min[1], true);
            p = Clipping::clipPolygonAxisAlignedPlane(p, 1, orthoBox.max[1], false);
        }

        float maxZ = -12057135;
        float minZ = 0213650235;

        for (auto& p : trisp)
        {
            for (auto& v : p)
            {
                minZ = std::min(minZ, v[2]);
                maxZ = std::max(maxZ, v[2]);
            }
        }

        std::swap(minZ, maxZ);
        minZ = -minZ;
        maxZ = -maxZ;

        //    std::cout << "min max Z " << minZ << " " << maxZ << std::endl;
        //    std::cout << "ortho min max Z " << orthoMin[2] << " " << orthoMax[2] << std::endl;


        orthoBox.min[2] = minZ;
        orthoBox.max[2] = maxZ;
    }

    //    this->cam.setProj(orthoBox);
    //    this->cam.setProj(
    //                orthoMin[0] ,orthoMax[0],
    //                orthoMin[1] ,orthoMax[1],
    //                orthoMin[2] ,orthoMax[2]
    //                );
}*/


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

void DirectionalLightRenderer::render(vk::CommandBuffer cmd)
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
    pushConstantObject.model            = light->model;
    pushConstantObject.specularCol      = make_vec4(light->getColorSpecular(), 1.f);
    pushConstantObject.diffuseCol       = make_vec4(light->getColorDiffuse(), light->getIntensity());
    pushConstantObject.ambientIntensity = light->getAmbientIntensity();
    // pushConstant(cmd, vk::ShaderStageFlagBits::eVertex, sizeof(mat4), data(translate(light->position)));
    pushConstant(cmd, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, sizeof(pushConstantObject),
                 &pushConstantObject, 0);
}

// shadow renderer
void DirectionalShadowLightRenderer::destroy()
{
    Pipeline::destroy();
    uniformBufferFS.destroy();
}

void DirectionalShadowLightRenderer::render(vk::CommandBuffer cmd)
{
    bindDescriptorSet(cmd, descriptorSet);
    // vk::Viewport vp(position[0], position[1], size[0], size[1]);
    // cmd.setViewport(0, vp);
    blitMesh.render(cmd);
}

void DirectionalShadowLightRenderer::init(VulkanBase& vulkanDevice, VkRenderPass renderPass, std::string fragmentShader)
{
    PipelineBase::init(vulkanDevice, 1);
    addDescriptorSetLayout({{0, {11, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {1, {12, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {2, {13, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {3, {14, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {4, {15, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {5, {16, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}},
                            {6, {17, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment}}});

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

    uniformBufferFS.init(*base, &uboFS, sizeof(uboFS));
}

void DirectionalShadowLightRenderer::updateUniformBuffers(vk::CommandBuffer cmd, mat4 proj, mat4 view, bool debug)
{
    uboFS.proj  = proj;
    uboFS.view  = view;
    uboFS.debug = debug;
    uniformBufferFS.update(cmd, sizeof(uboFS), &uboFS);
}
void DirectionalShadowLightRenderer::createAndUpdateDescriptorSet(
    Saiga::Vulkan::Memory::ImageMemoryLocation* diffuse, Saiga::Vulkan::Memory::ImageMemoryLocation* specular,
    Saiga::Vulkan::Memory::ImageMemoryLocation* normal, Saiga::Vulkan::Memory::ImageMemoryLocation* additional,
    Saiga::Vulkan::Memory::ImageMemoryLocation* depth, Saiga::Vulkan::Memory::ImageMemoryLocation* shadowmap)
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

    vk::DescriptorImageInfo shadowDescriptorInfo = shadowmap->data.get_descriptor_info();
    shadowDescriptorInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    shadowDescriptorInfo.setImageView(shadowmap->data.view);
    shadowDescriptorInfo.setSampler(shadowmap->data.sampler);


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
            vk::WriteDescriptorSet(descriptorSet, 16, 0, 1, vk::DescriptorType::eCombinedImageSampler,
                                   &shadowDescriptorInfo, nullptr, nullptr),
            vk::WriteDescriptorSet(descriptorSet, 17, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr,
                                   &uboFSDescriptorInfo, nullptr),

        },
        nullptr);
}

void DirectionalShadowLightRenderer::updateImageMemoryLocations(Memory::ImageMemoryLocation* diffuse,
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

void DirectionalShadowLightRenderer::createAndUpdateDescriptorSetShadow(Memory::ImageMemoryLocation* shadowmap)
{
    descriptorSet = createDescriptorSet();


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

        },
        nullptr);
}
void DirectionalShadowLightRenderer::pushLight(vk::CommandBuffer cmd, std::shared_ptr<DirectionalLight> light,
                                               Camera* cam)
{
    // const mat4 biasMatrix = make_mat4(0.5, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.5, 0.5,
    // 0.5, 1.0); light->shadowCamera.setProj(light->orthoBoxes[0]); mat4 shadow = biasMatrix * light->shadowCamera.proj
    // * light->shadowCamera.view * cam->model;



    pushConstantObject.model            = light->model;
    pushConstantObject.depthBiasMV      = light->viewToLightTransform(*cam, light->shadowCamera);
    pushConstantObject.specularCol      = make_vec4(light->getColorSpecular(), 1.f);
    pushConstantObject.diffuseCol       = make_vec4(light->getColorDiffuse(), light->getIntensity());
    pushConstantObject.direction        = make_vec4(light->getDirection(), 0.f);
    pushConstantObject.ambientIntensity = light->getAmbientIntensity();
    // pushConstant(cmd, vk::ShaderStageFlagBits::eVertex, sizeof(mat4), data(translate(light->position)));
    pushConstant(cmd, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, sizeof(pushConstantObject),
                 &pushConstantObject, 0);
}

}  // namespace Lighting

}  // namespace Vulkan
}  // namespace Saiga
