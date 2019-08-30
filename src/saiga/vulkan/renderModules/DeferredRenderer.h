

#pragma once

//#include "saiga/core/geometry/triangle_mesh.h"
//#include "saiga/vulkan/Base.h"
//#include "saiga/vulkan/VulkanAsset.h"
//#include "saiga/vulkan/VulkanBuffer.hpp"
//#include "saiga/vulkan/buffer/UniformBuffer.h"
//#include "saiga/vulkan/pipeline/Pipeline.h"
//#include "saiga/vulkan/svulkan.h"
//#include "saiga/vulkan/texture/Texture.h"

#include "saiga/vulkan/renderModules/AssetRenderer.h"
#include "saiga/vulkan/renderModules/LineAssetRenderer.h"
#include "saiga/vulkan/renderModules/PointCloudRenderer.h"
#include "saiga/vulkan/renderModules/TextureDisplay.h"
#include "saiga/vulkan/renderModules/TexturedAssetRenderer.h"

namespace Saiga
{
namespace Vulkan
{
//!
//! \brief The UniversalAssetRenderer class
//! contains deferred and forward renderers for easier handling
//!
class SAIGA_VULKAN_API UniversalAssetRenderer
{
   public:
    ~UniversalAssetRenderer() { destroy(); }

    void destroy();

    bool bindDeferred(vk::CommandBuffer cmd);
    bool bindForward(vk::CommandBuffer cmd);

    void pushModelDeferred(VkCommandBuffer cmd, mat4 model);
    void pushModelForward(VkCommandBuffer cmd, mat4 model);

    void updateUniformBuffersDeferred(vk::CommandBuffer cmd, mat4 view, mat4 proj);
    void updateUniformBuffersForward(vk::CommandBuffer cmd, mat4 view, mat4 proj);

    void init(Saiga::Vulkan::VulkanBase& vulkanDevice, VkRenderPass deferredPass, VkRenderPass forwardPass);

    void reload();

   private:
    Saiga::Vulkan::DeferredAssetRenderer deferredRenderer;
    Saiga::Vulkan::AssetRenderer forwardRenderer;
};

//!
//! \brief The UniversalLineAssetRenderer class
//! contains deferred and forward renderers for easier handling
//!
class SAIGA_VISION_API UniversalLineAssetRenderer
{
   public:
    ~UniversalLineAssetRenderer() { destroy(); }

    void destroy();

    bool bindDeferred(vk::CommandBuffer cmd);
    bool bindForward(vk::CommandBuffer cmd);

    void pushModelDeferred(vk::CommandBuffer cmd, mat4 model, vec4 color = make_vec4(1));
    void pushModelForward(vk::CommandBuffer cmd, mat4 model, vec4 color = make_vec4(1));

    void updateUniformBuffersDeferred(vk::CommandBuffer cmd, mat4 view, mat4 proj);
    void updateUniformBuffersForward(vk::CommandBuffer cmd, mat4 view, mat4 proj);

    void init(Saiga::Vulkan::VulkanBase& vulkanDevice, vk::RenderPass deferredPass, vk::RenderPass forwardPass,
              float lineWidth);

    void reload();

   private:
    Saiga::Vulkan::DeferredLineAssetRenderer deferredRenderer;
    Saiga::Vulkan::LineAssetRenderer forwardRenderer;
};

//!
//! \brief The UniversalPointCloudRenderer class
//! contains deferred and forward renderers for easier handling
//!
class SAIGA_VULKAN_API UniversalPointCloudRenderer
{
   public:
    ~UniversalPointCloudRenderer() { destroy(); }

    void destroy();

    bool bindDeferred(vk::CommandBuffer cmd);
    bool bindForward(vk::CommandBuffer cmd);

    void pushModelDeferred(VkCommandBuffer cmd, mat4 model);
    void pushModelForward(VkCommandBuffer cmd, mat4 model);

    void updateUniformBuffersDeferred(vk::CommandBuffer cmd, mat4 view, mat4 proj);
    void updateUniformBuffersForward(vk::CommandBuffer cmd, mat4 view, mat4 proj);

    void init(Saiga::Vulkan::VulkanBase& vulkanDevice, VkRenderPass deferredPass, VkRenderPass forwardPass,
              float pointSize);

    void reload();

   private:
    Saiga::Vulkan::DeferredPointCloudRenderer deferredRenderer;
    Saiga::Vulkan::PointCloudRenderer forwardRenderer;
};

}  // namespace Vulkan
}  // namespace Saiga
