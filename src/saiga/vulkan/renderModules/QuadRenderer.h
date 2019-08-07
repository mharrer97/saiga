/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */


#pragma once

#include "saiga/core/geometry/triangle_mesh.h"
#include "saiga/vulkan/Base.h"
#include "saiga/vulkan/VulkanAsset.h"
#include "saiga/vulkan/VulkanBuffer.hpp"
#include "saiga/vulkan/buffer/UniformBuffer.h"
#include "saiga/vulkan/pipeline/Pipeline.h"
#include "saiga/vulkan/svulkan.h"

namespace Saiga
{
namespace Vulkan
{
class SAIGA_VULKAN_API QuadRenderer : public Pipeline
{
   public:
    // Change these strings before calling 'init' to use your own shaders
    std::string vertexShader = "vulkan/quad.vert";
    std::string fragmentShader = "vulkan/quad.frag";

    ~QuadRenderer() { destroy(); }
    void destroy();

    bool bind(vk::CommandBuffer cmd);
    void render(vk::CommandBuffer cmd);

    void pushModel(VkCommandBuffer cmd, mat4 model);
    void updateUniformBuffers(vk::CommandBuffer cmd, mat4 view, mat4 proj);

    void init(Saiga::Vulkan::VulkanBase& vulkanDevice, VkRenderPass renderPass);

    void prepareUniformBuffers(Saiga::Vulkan::VulkanBase* vulkanDevice);
    //    void preparePipelines(VkPipelineCache pipelineCache, VkRenderPass renderPass);
    void setupLayoutsAndDescriptors();

    void createAndUpdateDescriptorSet(vk::ImageView diffuse, vk::ImageView specular, vk::ImageView normal, vk::ImageView additional);

   private:
    void setupColorAttachmentSampler();

    struct UBOVS
    {
        mat4 projection;
        mat4 modelview;
        vec4 lightPos;
    } uboVS;

    Saiga::Vulkan::VulkanVertexColoredAsset quad;

    UniformBuffer uniformBufferVS;
    StaticDescriptorSet descriptorSet;

    vk::Sampler colorSampler;
};

}  // namespace Vulkan
}  // namespace Saiga
