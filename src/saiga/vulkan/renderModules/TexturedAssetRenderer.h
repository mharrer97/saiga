﻿/**
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
#include "saiga/vulkan/texture/Texture.h"

namespace Saiga
{
namespace Vulkan
{
class SAIGA_VULKAN_API TexturedAssetRenderer : public Pipeline
{
   public:
    using VertexType = VertexNTD;

    ~TexturedAssetRenderer() { destroy(); }
    void destroy();



    void bindTexture(vk::CommandBuffer cmd, vk::DescriptorSet ds);

    void pushModel(vk::CommandBuffer cmd, mat4 model);
    void updateUniformBuffers(vk::CommandBuffer cmd, mat4 view, mat4 proj);

    void init(Saiga::Vulkan::VulkanBase& vulkanDevice, VkRenderPass renderPass,
              const std::string& vertShader = "vulkan/texturedAsset.vert",
              const std::string& fragShader = "vulkan/texturedAsset.frag");

    void prepareUniformBuffers(Saiga::Vulkan::VulkanBase* vulkanDevice);
    void setupLayoutsAndDescriptors();

    StaticDescriptorSet createAndUpdateDescriptorSet(Texture& texture);

   private:
    struct UBOVS
    {
        mat4 projection;
        mat4 modelview;
        vec4 lightPos;
    } uboVS;

    UniformBuffer uniformBufferVS;
};


class SAIGA_VULKAN_API DeferredTexturedAssetRenderer : public Pipeline
{
   public:
    using VertexType = VertexNTD;

    ~DeferredTexturedAssetRenderer() { destroy(); }
    void destroy();



    void bindTexture(vk::CommandBuffer cmd, vk::DescriptorSet ds);

    void pushModel(vk::CommandBuffer cmd, mat4 model);
    void updateUniformBuffers(vk::CommandBuffer cmd, mat4 view, mat4 proj);

    void init(Saiga::Vulkan::VulkanBase& vulkanDevice, VkRenderPass renderPass,
              const std::string& vertShader = "vulkan/texturedAsset.vert",
              const std::string& fragShader = "vulkan/texturedAssetDeferred.frag");

    void prepareUniformBuffers(Saiga::Vulkan::VulkanBase* vulkanDevice);
    void setupLayoutsAndDescriptors();

    StaticDescriptorSet createAndUpdateDescriptorSet(Texture& texture);

   private:
    struct UBOVS
    {
        mat4 projection;
        mat4 modelview;
        vec4 lightPos;
    } uboVS;

    UniformBuffer uniformBufferVS;
};

}  // namespace Vulkan
}  // namespace Saiga
