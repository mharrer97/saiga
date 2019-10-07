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



class SAIGA_VULKAN_API BoxLight : public Light
{
    friend class DeferredLighting;

   protected:
    // std::shared_ptr<CubeShadowmap> shadowmap;

   public:
    // float shadowNearPlane = 0.1f;
    // PerspectiveCamera shadowCamera;
    OrthographicCamera shadowCamera;


    BoxLight();
    ~BoxLight() {}



    // void bindUniforms(std::shared_ptr<PointLightShader> shader, Camera* shadowCamera);


    // float getRadius() const;
    // void setRadius(float value);


    // void createShadowMap(int w, int h, ShadowQuality quality = ShadowQuality::LOW);

    // void bindFace(int face);
    // void calculateCamera(int face);

    void setView(vec3 pos, vec3 target, vec3 up);

    void calculateCamera();

    bool cullLight(Camera* camera);
    // bool renderShadowmap(DepthFunction f, UniformBuffer& shadowCameraBuffer);
    // void renderImGui();
};


class SAIGA_VULKAN_API BoxLightRenderer : public Pipeline
{
   public:
    using VertexType = Vertex;



    // Change these strings before calling 'init' to use your own shaders
    std::string vertexShader = "vulkan/lighting/attenuatedLight.vert";

    ~BoxLightRenderer() { destroy(); }
    void destroy();


    /**
     * Render the texture at the given pixel position and size
     */
    void render(vk::CommandBuffer cmd, std::shared_ptr<BoxLight> light);



    void init(Saiga::Vulkan::VulkanBase& vulkanDevice, VkRenderPass renderPass, std::string fragmentShader);

    void updateUniformBuffers(vk::CommandBuffer, mat4 proj, mat4 view, bool debug);

    void createAndUpdateDescriptorSet(Saiga::Vulkan::Memory::ImageMemoryLocation* diffuse,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* specular,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* normal,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* additional,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* depth);
    void pushLight(vk::CommandBuffer cmd, std::shared_ptr<BoxLight> light, Camera* cam);

   private:
    struct UBOFS
    {
        mat4 proj;
        mat4 view;
        bool debug;

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
        vec4 specularCol;
        vec4 diffuseCol;
    } pushConstantObject;

    UniformBuffer uniformBufferVS;
    UniformBuffer uniformBufferFS;

    Saiga::Vulkan::StaticDescriptorSet descriptorSet;
    Saiga::Vulkan::VulkanVertexAsset lightMesh;
};
}  // namespace Lighting
}  // namespace Vulkan
}  // namespace Saiga



#include "saiga/core/camera/all.h"
#include "saiga/core/math/math.h"
#include "saiga/core/util/assert.h"
