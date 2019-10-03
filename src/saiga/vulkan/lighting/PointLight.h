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



class SAIGA_VULKAN_API PointLight : public AttenuatedLight
{
    friend class DeferredLighting;

   protected:
    // std::shared_ptr<CubeShadowmap> shadowmap;

   public:
    // float shadowNearPlane = 0.1f;
    // PerspectiveCamera shadowCamera;


    PointLight();
    virtual ~PointLight() {}

    PointLight& operator=(const PointLight& light);



    // void bindUniforms(std::shared_ptr<PointLightShader> shader, Camera* shadowCamera);


    float getRadius() const;
    virtual void setRadius(float value);


    // void createShadowMap(int w, int h, ShadowQuality quality = ShadowQuality::LOW);

    // void bindFace(int face);
    // void calculateCamera(int face);


    bool cullLight(Camera* camera);
    // bool renderShadowmap(DepthFunction f, UniformBuffer& shadowCameraBuffer);
    // void renderImGui();
};


class SAIGA_VULKAN_API PointLightRenderer : public Pipeline
{
   public:
    using VertexType = Vertex;

    // Change these strings before calling 'init' to use your own shaders
    std::string vertexShader = "vulkan/lighting/attenuatedLight.vert";

    ~PointLightRenderer() { destroy(); }
    void destroy();


    /**
     * Render the texture at the given pixel position and size
     */
    void render(vk::CommandBuffer cmd, std::shared_ptr<PointLight> light);



    void init(Saiga::Vulkan::VulkanBase& vulkanDevice, VkRenderPass renderPass, std::string fragmentShader);

    void updateUniformBuffers(vk::CommandBuffer, mat4 proj, mat4 view, bool debug);

    void createAndUpdateDescriptorSet(Saiga::Vulkan::Memory::ImageMemoryLocation* diffuse,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* specular,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* normal,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* additional,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* depth);
    void pushLight(vk::CommandBuffer cmd, std::shared_ptr<PointLight> light);

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
        vec4 pos;
        vec4 attenuation;
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
//#include "saiga/vulkan/CommandPool.h"
//#include "saiga/vulkan/FrameSync.h"
//#include "saiga/vulkan/Queue.h"
//#include "saiga/vulkan/Renderer.h"
//#include "saiga/vulkan/VulkanAsset.h"
//#include "saiga/vulkan/buffer/ColorBuffer.h"
//#include "saiga/vulkan/buffer/DepthBuffer.h"
//#include "saiga/vulkan/buffer/Framebuffer.h"
//#include "saiga/vulkan/renderModules/QuadRenderer.h"
//#include "saiga/vulkan/window/Window.h"

namespace Saiga
{
namespace Vulkan
{
namespace Lighting
{
class SAIGA_VULKAN_API OldPointLight
{
   public:
    OldPointLight(vec3 pos, vec3 dir, float angle);

    ~OldPointLight() {}

    void setPosition(vec3 pos);
    vec3 setDirection(vec3 dir);
    float setAngle(float angle);

    vec3 getPosition();
    vec3 getDirection();
    float getAngle();


    float angle;  // TODO changeto private
    vec3 light_dir;

   private:
    vec3 light_pos;
};
}  // namespace Lighting
}  // namespace Vulkan
}  // namespace Saiga
