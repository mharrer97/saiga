/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 *
 * Created by Mathias Harrer: mathias.mh.harrer@fau.de
 */

#pragma once
#include "saiga/export.h"
#include "saiga/vulkan/VulkanAsset.h"
#include "saiga/vulkan/pipeline/Pipeline.h"

namespace Saiga
{
namespace Vulkan
{
namespace RTX
{
class SAIGA_VULKAN_API Denoiser : public Pipeline
{
   public:
    using VertexType           = VertexNC;
    std::string vertexShader   = "vulkan/raytracing/denoise.vert";
    std::string fragmentShader = "vulkan/raytracing/denoise.frag";

    ~Denoiser() { destroy(); }
    void destroy();

    void render(vk::CommandBuffer cmd);

    void init(Saiga::Vulkan::VulkanBase& vulkanDevice, VkRenderPass renderPass);

    void updateUniformBuffers(vk::CommandBuffer cmd, int maxKernelSize);


    void createAndUpdateDescriptorSet(Saiga::Vulkan::Memory::ImageMemoryLocation* rtx,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* normal,
                                      Saiga::Vulkan::Memory::ImageMemoryLocation* data);

   private:
    struct UBOFS
    {
        int maxKernelSize;

    } uboFS;

    UniformBuffer uniformBufferFS;

    Saiga::Vulkan::StaticDescriptorSet descriptorSet;
    Saiga::Vulkan::VulkanVertexColoredAsset blitMesh;
};
}  // namespace RTX
}  // namespace Vulkan
}  // namespace Saiga
