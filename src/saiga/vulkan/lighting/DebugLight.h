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
#include "saiga/vulkan/lighting/PointLight.h"
#include "saiga/vulkan/lighting/SpotLight.h"

namespace Saiga
{
namespace Vulkan
{
namespace Lighting
{
class SAIGA_VULKAN_API DebugLightRenderer : public Pipeline
{
   public:
    using VertexType = Vertex;

    // Change these strings before calling 'init' to use your own shaders
    std::string vertexShader = "vulkan/lighting/attenuatedLight.vert";

    ~DebugLightRenderer() { destroy(); }
    void destroy();


    /**
     * Render the texture at the given pixel position and size
     */
    void render(vk::CommandBuffer cmd, std::shared_ptr<SpotLight> light);
    void render(vk::CommandBuffer cmd, std::shared_ptr<PointLight> light);



    void init(Saiga::Vulkan::VulkanBase& vulkanDevice, VkRenderPass renderPass, std::string fragmentShader,
              float lineWidth);

    void updateUniformBuffers(vk::CommandBuffer, mat4 proj, mat4 view);

    void createAndUpdateDescriptorSet(Saiga::Vulkan::Memory::ImageMemoryLocation* diffuse,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* specular,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* normal,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* additional,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* depth);

    void pushLight(vk::CommandBuffer cmd, std::shared_ptr<AttenuatedLight> light);


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
    } pushConstantObject;

    UniformBuffer uniformBufferVS;
    UniformBuffer uniformBufferFS;

    Saiga::Vulkan::StaticDescriptorSet descriptorSet;
    Saiga::Vulkan::VulkanVertexAsset lightMeshSpot;
    Saiga::Vulkan::VulkanVertexAsset lightMeshPoint;
};
}  // namespace Lighting
}  // namespace Vulkan
}  // namespace Saiga
