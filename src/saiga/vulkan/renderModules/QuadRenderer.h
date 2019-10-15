/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 *
 * Created by Mathias Harrer: mathias.mh.harrer@fau.de
 */

// TODO: delete whole file. deprecated

#pragma once

#include "saiga/core/geometry/triangle_mesh.h"
#include "saiga/vulkan/Base.h"
#include "saiga/vulkan/VulkanAsset.h"
#include "saiga/vulkan/VulkanBuffer.hpp"
#include "saiga/vulkan/buffer/UniformBuffer.h"
#include "saiga/vulkan/pipeline/Pipeline.h"
#include "saiga/vulkan/svulkan.h"
#include "saiga/vulkan/texture/Texture.h"

namespace Saiga
{
namespace Vulkan
{
class SAIGA_VULKAN_API QuadRenderer : public Pipeline
{
   public:
    using VertexType = VertexNC;


    ~QuadRenderer() { destroy(); }
    void destroy();


    /**
     * Render the texture at the given pixel position and size
     */
    void render(vk::CommandBuffer cmd, vec2 position, vec2 size);



    void init(Saiga::Vulkan::VulkanBase& vulkanDevice, VkRenderPass renderPass);

    void updateUniformBuffers(vk::CommandBuffer cmd, mat4 proj, mat4 view, vec4 lightColor, vec4 lightDirection,
                              bool debug, float intensity);

    void createAndUpdateDescriptorSet(Saiga::Vulkan::Memory::ImageMemoryLocation* diffuse,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* specular,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* normal,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* additional,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* depth);

   private:
    struct UBOVS
    {
        mat4 proj;
        mat4 view;
        vec4 lightDir;
        vec4 lightColor;
        float intensity;
        bool debug;

    } uboVS;

    UniformBuffer uniformBufferVS;


    Saiga::Vulkan::VulkanVertexColoredAsset blitMesh;
    Saiga::Vulkan::StaticDescriptorSet descriptorSet;
};



}  // namespace Vulkan
}  // namespace Saiga
