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
/*class SAIGA_OPENGL_API PointLightShader : public AttenuatedLightShader
{
   public:
    GLint location_shadowPlanes;

    virtual void checkUniforms();

    void uploadShadowPlanes(float f, float n);
};*/



class SAIGA_VULKAN_API SpotLight : public AttenuatedLight
{
    friend class DeferredLighting;

   protected:
    // std::shared_ptr<CubeShadowmap> shadowmap;
    float openingAngle = 90.f;
    vec3 direction     = vec3(0.f, -1.f, 0.f);

   public:
    float shadowNearPlane = 0.1f;
    // PerspectiveCamera shadowCamera;
    PerspectiveCamera shadowCamera;
    bool shadowMapInitialized = false;
    std::shared_ptr<SimpleShadowmap> shadowmap;
    StaticDescriptorSet shadowMapDescriptor;

    SpotLight();
    virtual ~SpotLight() {}

    SpotLight& operator=(const SpotLight& light);



    void createShadowMap(VulkanBase& vulkanDevice, int w, int h,
                         vk::RenderPass shadowPass);  //, ShadowQuality quality = ShadowQuality::LOW);
    void destroyShadowMap();


    // void bindUniforms(std::shared_ptr<PointLightShader> shader, Camera* shadowCamera);
    void recalculateScale();


    float getRadius() const;
    virtual void setRadius(float value);

    float getAngle() const { return openingAngle; }
    virtual void setAngle(float value);

    vec3 getDirection() const;
    virtual void setDirection(vec3 value);

    // void createShadowMap(int w, int h, ShadowQuality quality = ShadowQuality::LOW);

    // void bindFace(int face);
    // void calculateCamera(int face);

    void calculateCamera();

    bool cullLight(Camera* camera);
    // bool renderShadowmap(DepthFunction f, UniformBuffer& shadowCameraBuffer);
    // void renderImGui();
};


class SAIGA_VULKAN_API SpotLightRenderer : public Pipeline
{
   public:
    using VertexType = Vertex;

    // Change these strings before calling 'init' to use your own shaders
    std::string vertexShader = "vulkan/lighting/attenuatedLight.vert";

    ~SpotLightRenderer() { destroy(); }
    void destroy();


    /**
     * Render the texture at the given pixel position and size
     */
    void render(vk::CommandBuffer cmd, std::shared_ptr<SpotLight> light);



    void init(Saiga::Vulkan::VulkanBase& vulkanDevice, VkRenderPass renderPass, std::string fragmentShader);

    void updateUniformBuffers(vk::CommandBuffer, mat4 proj, mat4 view);

    void createAndUpdateDescriptorSet(Saiga::Vulkan::Memory::ImageMemoryLocation* diffuse,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* specular,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* normal,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* additional,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* depth);
    void pushLight(vk::CommandBuffer cmd, std::shared_ptr<SpotLight> light);

   private:
    struct UBOFS
    {
        mat4 proj;
        mat4 view;
    } uboFS;

    struct UBOVS
    {
        mat4 proj;
        mat4 view;
    } uboVS;

    struct PCO  // TODO evtl use separate pushconstants in each shader stage?
    {
        mat4 model;
        vec4 pos;
        vec4 attenuation;
        vec4 dir;
        vec4 specularCol;
        vec4 diffuseCol;
        float openingAngle;
    } pushConstantObject;

    UniformBuffer uniformBufferVS;
    UniformBuffer uniformBufferFS;

    Saiga::Vulkan::StaticDescriptorSet descriptorSet;
    Saiga::Vulkan::VulkanVertexAsset lightMesh;
    Saiga::Vulkan::VulkanVertexAsset lightMeshIco;
};

class SAIGA_VULKAN_API SpotShadowLightRenderer : public Pipeline
{
   public:
    using VertexType = Vertex;

    // Change these strings before calling 'init' to use your own shaders
    std::string vertexShader = "vulkan/lighting/attenuatedLight.vert";

    ~SpotShadowLightRenderer() { destroy(); }
    void destroy();


    /**
     * Render the texture at the given pixel position and size
     */
    void render(vk::CommandBuffer cmd, std::shared_ptr<SpotLight> light, DescriptorSet& descriptorSet);



    void init(Saiga::Vulkan::VulkanBase& vulkanDevice, VkRenderPass renderPass, std::string fragmentShader);

    void updateUniformBuffers(vk::CommandBuffer, mat4 proj, mat4 view);

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
    void pushLight(vk::CommandBuffer cmd, std::shared_ptr<SpotLight> light, Camera* cam);

   private:
    struct UBOFS
    {
        mat4 proj;
        mat4 view;
    } uboFS;

    struct UBOVS
    {
        mat4 proj;
        mat4 view;
    } uboVS;

    struct PCO  // TODO evtl use separate pushconstants in each shader stage?
    {
        mat4 model;
        mat4 depthBiasMV;
        vec4 pos;
        vec4 attenuation;
        vec4 dir;
        vec4 specularCol;
        vec4 diffuseCol;
        float openingAngle;
    } pushConstantObject;

    UniformBuffer uniformBufferVS;
    UniformBuffer uniformBufferFS;

    Saiga::Vulkan::VulkanVertexAsset lightMesh;
    Saiga::Vulkan::VulkanVertexAsset lightMeshIco;

    Saiga::Vulkan::Memory::ImageMemoryLocation* diffuseLocation;
    Saiga::Vulkan::Memory::ImageMemoryLocation* specularLocation;
    Saiga::Vulkan::Memory::ImageMemoryLocation* normalLocation;
    Saiga::Vulkan::Memory::ImageMemoryLocation* additionalLocation;
    Saiga::Vulkan::Memory::ImageMemoryLocation* depthLocation;
};

}  // namespace Lighting
}  // namespace Vulkan
}  // namespace Saiga
