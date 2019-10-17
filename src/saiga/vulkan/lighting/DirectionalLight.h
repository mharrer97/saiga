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
    std::shared_ptr<SimpleShadowmap> shadowmap;

   public:
    // float shadowNearPlane = 0.1f;
    // PerspectiveCamera shadowCamera;
    OrthographicCamera shadowCamera;
    bool shadowMapInitialized = false;

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

    void setDirection(vec3 dir);

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
    } pushConstantObject;

    UniformBuffer uniformBufferFS;

    Saiga::Vulkan::StaticDescriptorSet descriptorSet;
    Saiga::Vulkan::VulkanVertexColoredAsset blitMesh;
};



}  // namespace Lighting
}  // namespace Vulkan
}  // namespace Saiga
