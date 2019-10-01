/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 *
 * Created by Mathias Harrer: mathias.mh.harrer@fau.de
 *
 */

#pragma once

#include "saiga/core/camera/camera.h"
#include "saiga/core/geometry/triangle_mesh.h"
#include "saiga/core/model/objModelLoader.h"
#include "saiga/vulkan/Base.h"
#include "saiga/vulkan/Shader/all.h"
#include "saiga/vulkan/Vertex.h"
#include "saiga/vulkan/VulkanAsset.h"
#include "saiga/vulkan/VulkanBuffer.hpp"
#include "saiga/vulkan/buffer/UniformBuffer.h"
#include "saiga/vulkan/lighting/Light.h"
#include "saiga/vulkan/pipeline/Pipeline.h"
#include "saiga/vulkan/svulkan.h"

namespace Saiga
{
namespace Vulkan
{
namespace Lighting
{
/*class SAIGA_OPENGL_API AttenuatedLightShader : public LightShader
{
   public:
    GLint location_attenuation;

    virtual void checkUniforms();
    virtual void uploadA(vec3& attenuation, float cutoffRadius);
};*/

/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */



namespace AttenuationPresets
{
static const vec3 NONE = vec3(1, 0, 0);  // Cutoff = 1

static const vec3 LinearWeak   = vec3(1, 0.5, 0);  // Cutoff = 0.666667
static const vec3 Linear       = vec3(1, 1, 0);    // Cutoff = 0.5
static const vec3 LinearStrong = vec3(1, 4, 0);    // Cutoff = 0.2


static const vec3 QuadraticWeak   = vec3(1, 0.5, 0.5);  // Cutoff = 0.5
static const vec3 Quadratic       = vec3(1, 1, 1);      // Cutoff = 0.333333
static const vec3 QuadraticStrong = vec3(1, 2, 4);      // Cutoff = 0.142857
}  // namespace AttenuationPresets


class SAIGA_VULKAN_API AttenuatedLight : public Light
{
    friend class DeferredLighting;

   private:
   protected:
    /**
     * Quadratic attenuation of the form:
     * I = i/(a*x*x+b*x+c)
     *
     * It is stored in the format:
     * vec3(c,b,a)
     *
     * Note: The attenuation is independent of the radius.
     * x = d / r, where d is the distance to the light
     *
     * This normalized attenuation makes it easy to scale lights without having to change the attenuation
     *
     */

    vec3 attenuation = AttenuationPresets::Quadratic;

    /**
     * Distance after which the light intensity is clamped to 0.
     * The shadow volumes should be constructed so that they closely contain
     * all points up to the cutoffradius.
     */
    float cutoffRadius = 10.f;

   public:
    AttenuatedLight();
    virtual ~AttenuatedLight() {}
    AttenuatedLight& operator=(const AttenuatedLight& light);

    // evaluates the attenuation formula at a given radius
    float evaluateAttenuation(float distance);

    // void bindUniforms(std::shared_ptr<AttenuatedLightShader> shader, Camera* cam);


    float getRadius() const;
    virtual void setRadius(float value);

    vec3 getAttenuation() const;
    float getAttenuation(float r);
    void setAttenuation(const vec3& value);

    // void createShadowMap(int resX, int resY);

    // void bindFace(int face);
    // void calculateCamera(int face);


    bool cullLight(Camera* cam);
    // void renderImGui();
};


class SAIGA_VULKAN_API AttenuatedLightRenderer : public Pipeline
{
   public:
    using VertexType = VertexNC;

    // Change these strings before calling 'init' to use your own shaders
    std::string vertexShader   = "vulkan/quadRenderer.vert";              //"vulkan/coloredAsset.vert";
    std::string fragmentShader = "vulkan/lighting/attenuatedLight.frag";  //"vulkan/coloredAssetDeferred.frag";

    ~AttenuatedLightRenderer() { destroy(); }
    void destroy();


    /**
     * Render the texture at the given pixel position and size
     */
    void render(vk::CommandBuffer cmd, std::shared_ptr<AttenuatedLight> light);



    void init(Saiga::Vulkan::VulkanBase& vulkanDevice, VkRenderPass renderPass);

    void updateUniformBuffers(vk::CommandBuffer, mat4 proj, mat4 view, vec4 lightPosition, float intensity, bool debug);

    void createAndUpdateDescriptorSet(Saiga::Vulkan::Memory::ImageMemoryLocation* diffuse,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* specular,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* normal,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* additional,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* depth);
    void pushPosition(vk::CommandBuffer cmd, vec4 pos);

   private:
    struct UBOVS
    {
        mat4 proj;
        mat4 view;
        vec4 lightPos;
        float intensity;
        bool debug;

    } uboVS;

    struct PCO
    {
        vec4 pos;
    } pushConstantObject;

    UniformBuffer uniformBufferVS;


    Saiga::Vulkan::StaticDescriptorSet descriptorSet;
    Saiga::Vulkan::VulkanVertexColoredAsset lightMesh;
};

}  // namespace Lighting
}  // namespace Vulkan
}  // namespace Saiga
