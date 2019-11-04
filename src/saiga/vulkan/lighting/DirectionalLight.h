/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 *
 * Created by Mathias Harrer: mathias.mh.harrer@fau.de
 */


#pragma once

#include "saiga/core/camera/camera.h"
#include "saiga/vulkan/lighting/AttenuatedLight.h"
#include "saiga/vulkan/lighting/Shadowmap.h"

namespace Saiga
{
namespace Vulkan
{
namespace Lighting
{
class SAIGA_VULKAN_API DirectionalLight : public Light
{
    friend class DeferredLighting;

   protected:
    float ambientIntensity = 0.2f;
    // direction of the light in world space
    vec3 direction = vec3(-1, -1, -1);



    // relative split planes to the camera near and far plane
    // must be of size numCascades + 1
    // should start with 0 and end with 1
    // values between the first and the last indicate the split planes
    std::vector<float> depthCutsRelative;

    // actual split planes in view space depth
    // will be calculated from depthCutsRelative
    std::vector<float> depthCuts;

    // The size in world space units how big the interpolation region between two cascades is.
    // Larger values mean a smoother transition, but decreases performance, because more shadow samples need to be
    // fetched. Larger values also increase the size of each shadow frustum and therefore the quality may be reduceds.
    float cascadeInterpolateRange = 3.0f;

    // number of cascades for cascaded shadow mapping
    // 1 means normal shadow mapping
    int numCascades = 1;

   public:
    // bounding box of every cascade frustum
    std::vector<AABB> orthoBoxes;


    // float shadowNearPlane = 0.1f;
    // PerspectiveCamera shadowCamera;
    OrthographicCamera shadowCamera;
    bool shadowMapInitialized = false;
    std::shared_ptr<SimpleShadowmap> shadowmap;
    StaticDescriptorSet shadowMapDescriptor;

    DirectionalLight();
    ~DirectionalLight() {}



    // void bindUniforms(std::shared_ptr<PointLightShader> shader, Camera* shadowCamera);


    // float getRadius() const;
    // void setRadius(float value);


    void createShadowMap(VulkanBase& vulkanDevice, int w, int h,
                         vk::RenderPass shadowPass);  //, ShadowQuality quality = ShadowQuality::LOW);
    void destroyShadowMap();

    // void bindFace(int face);
    // void calculateCamera(int face);

    void setView(vec3 pos, vec3 target, vec3 up);

    /**
     * Sets the light direction in world coordinates.
     * Computes the view matrix for the shadow camera.
     */
    void setDirection(const vec3& dir);
    vec3 getDirection() const { return direction; }

    /**
     * Computes the left/right, bottom/top and near/far planes of the shadow volume so that,
     * it fits the given camera. This should be called every time the camera is translated
     * or rotated to be sure, that all visible objects have shadows.
     */
    void fitShadowToCamera(Camera* shadowCamera);

    /**
     * Computes the near plane of the shadow frustum so that all objects in the scene cast shadows.
     * Objects that do not lay in the camera frustum can cast shadows into it. That's why the method above
     * is not enough. This will overwrite the near plane, so it should be called after fitShadowToCamera.
     */
    // void fitNearPlaneToScene(AABB sceneBB);

    void setAmbientIntensity(float ai) { ambientIntensity = ai; }
    float getAmbientIntensity() { return ambientIntensity; }

    void calculateCamera();

    bool cullLight(Camera* camera);
    // bool renderShadowmap(DepthFunction f, UniformBuffer& shadowCameraBuffer);
    // void renderImGui();
};

class SAIGA_VULKAN_API DirectionalLightRenderer : public Pipeline
{
   public:
    using VertexType         = VertexNC;
    std::string vertexShader = "vulkan/lighting/directionalLight.vert";

    ~DirectionalLightRenderer() { destroy(); }
    void destroy();


    /**
     * Render the texture at the given pixel position and size
     */
    void render(vk::CommandBuffer cmd);



    void init(Saiga::Vulkan::VulkanBase& vulkanDevice, VkRenderPass renderPass, std::string fragmentShader);

    void updateUniformBuffers(vk::CommandBuffer cmd, mat4 proj, mat4 view, bool debug);

    void createAndUpdateDescriptorSet(Saiga::Vulkan::Memory::ImageMemoryLocation* diffuse,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* specular,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* normal,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* additional,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* depth);

    void pushLight(vk::CommandBuffer cmd, std::shared_ptr<DirectionalLight> light);

   private:
    struct UBOFS
    {
        mat4 proj;
        mat4 view;
        bool debug;

    } uboFS;

    struct PCO  // TODO evtl use separate pushconstants in each shader stage?
    {
        mat4 model;
        vec4 specularCol;
        vec4 diffuseCol;
        float ambientIntensity;
    } pushConstantObject;

    UniformBuffer uniformBufferFS;

    Saiga::Vulkan::StaticDescriptorSet descriptorSet;
    Saiga::Vulkan::VulkanVertexColoredAsset blitMesh;
};

class SAIGA_VULKAN_API DirectionalShadowLightRenderer : public Pipeline
{
   public:
    using VertexType         = VertexNC;
    std::string vertexShader = "vulkan/lighting/directionalLight.vert";

    ~DirectionalShadowLightRenderer() { destroy(); }
    void destroy();


    /**
     * Render the texture at the given pixel position and size
     */
    void render(vk::CommandBuffer cmd, DescriptorSet& dscriptorSet);



    void init(Saiga::Vulkan::VulkanBase& vulkanDevice, VkRenderPass renderPass, std::string fragmentShader);

    void updateUniformBuffers(vk::CommandBuffer cmd, mat4 proj, mat4 view, bool debug);

    void createAndUpdateDescriptorSet(Saiga::Vulkan::Memory::ImageMemoryLocation* diffuse,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* specular,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* normal,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* additional,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* depth,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* shadowmap);

    // alternative for createandupdatedescriptorset: save gbuffer imagememorylocations
    void updateImageMemoryLocations(Saiga::Vulkan::Memory::ImageMemoryLocation* diffuse,
                                    Saiga::Vulkan::Memory::ImageMemoryLocation* specular,
                                    Saiga::Vulkan::Memory::ImageMemoryLocation* normal,
                                    Saiga::Vulkan::Memory::ImageMemoryLocation* additional,
                                    Saiga::Vulkan::Memory::ImageMemoryLocation* depth);
    // only change shadowmap
    StaticDescriptorSet createAndUpdateDescriptorSetShadow(Saiga::Vulkan::Memory::ImageMemoryLocation* shadowmap);

    void pushLight(vk::CommandBuffer cmd, std::shared_ptr<DirectionalLight> light, Camera* cam);

   private:
    struct UBOFS
    {
        mat4 proj;
        mat4 view;
        bool debug;

    } uboFS;

    struct PCO  // TODO evtl use separate pushconstants in each shader stage?
    {
        mat4 model;
        mat4 depthBiasMV;
        vec4 specularCol;
        vec4 diffuseCol;
        vec4 direction;
        float ambientIntensity;
    } pushConstantObject;

    UniformBuffer uniformBufferFS;

    // Saiga::Vulkan::StaticDescriptorSet descriptorSet;
    Saiga::Vulkan::VulkanVertexColoredAsset blitMesh;

    Saiga::Vulkan::Memory::ImageMemoryLocation* diffuseLocation;
    Saiga::Vulkan::Memory::ImageMemoryLocation* specularLocation;
    Saiga::Vulkan::Memory::ImageMemoryLocation* normalLocation;
    Saiga::Vulkan::Memory::ImageMemoryLocation* additionalLocation;
    Saiga::Vulkan::Memory::ImageMemoryLocation* depthLocation;
};


}  // namespace Lighting
}  // namespace Vulkan
}  // namespace Saiga
