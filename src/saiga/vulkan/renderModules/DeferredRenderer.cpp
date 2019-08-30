#include "DeferredRenderer.h"

#if defined(SAIGA_OPENGL_INCLUDED)
#    error OpenGL was included somewhere.
#endif

namespace Saiga
{
namespace Vulkan
{
// Asset Renderer
void UniversalAssetRenderer::destroy()
{
    deferredRenderer.destroy();
    forwardRenderer.destroy();
}

void UniversalAssetRenderer::init(VulkanBase& vulkanDevice, VkRenderPass deferredPass, VkRenderPass forwardPass)
{
    deferredRenderer.init(vulkanDevice, deferredPass);
    forwardRenderer.init(vulkanDevice, forwardPass);
}

bool UniversalAssetRenderer::bindDeferred(vk::CommandBuffer cmd)
{
    return deferredRenderer.bind(cmd);
}

bool UniversalAssetRenderer::bindForward(vk::CommandBuffer cmd)
{
    return forwardRenderer.bind(cmd);
}

void UniversalAssetRenderer::pushModelDeferred(VkCommandBuffer cmd, mat4 model)
{
    deferredRenderer.pushModel(cmd, model);
}

void UniversalAssetRenderer::pushModelForward(VkCommandBuffer cmd, mat4 model)
{
    forwardRenderer.pushModel(cmd, model);
}

void UniversalAssetRenderer::updateUniformBuffersDeferred(vk::CommandBuffer cmd, mat4 view, mat4 proj)
{
    deferredRenderer.updateUniformBuffers(cmd, view, proj);
}

void UniversalAssetRenderer::updateUniformBuffersForward(vk::CommandBuffer cmd, mat4 view, mat4 proj)
{
    forwardRenderer.updateUniformBuffers(cmd, view, proj);
}

void UniversalAssetRenderer::reload()
{
    deferredRenderer.reload();
    forwardRenderer.reload();
}

// Line Asset Renderer
void UniversalLineAssetRenderer::destroy()
{
    deferredRenderer.destroy();
    forwardRenderer.destroy();
}

bool UniversalLineAssetRenderer::bindDeferred(vk::CommandBuffer cmd)
{
    return deferredRenderer.bind(cmd);
}

bool UniversalLineAssetRenderer::bindForward(vk::CommandBuffer cmd)
{
    return forwardRenderer.bind(cmd);
}

void UniversalLineAssetRenderer::pushModelDeferred(vk::CommandBuffer cmd, mat4 model, vec4 color)
{
    deferredRenderer.pushModel(cmd, model, color);
}

void UniversalLineAssetRenderer::pushModelForward(vk::CommandBuffer cmd, mat4 model, vec4 color)
{
    forwardRenderer.pushModel(cmd, model, color);
}

void UniversalLineAssetRenderer::updateUniformBuffersDeferred(vk::CommandBuffer cmd, mat4 view, mat4 proj)
{
    deferredRenderer.updateUniformBuffers(cmd, view, proj);
}

void UniversalLineAssetRenderer::updateUniformBuffersForward(vk::CommandBuffer cmd, mat4 view, mat4 proj)
{
    forwardRenderer.updateUniformBuffers(cmd, view, proj);
}

void UniversalLineAssetRenderer::init(VulkanBase& vulkanDevice, vk::RenderPass deferredPass, vk::RenderPass forwardPass,
                                      float lineWidth)
{
    deferredRenderer.init(vulkanDevice, deferredPass, lineWidth);
    forwardRenderer.init(vulkanDevice, forwardPass, lineWidth);
}

void UniversalLineAssetRenderer::reload()
{
    deferredRenderer.reload();
    forwardRenderer.reload();
}

// Point Cloud Renderer
void UniversalPointCloudRenderer::destroy()
{
    deferredRenderer.destroy();
    forwardRenderer.destroy();
}

void UniversalPointCloudRenderer::init(VulkanBase& vulkanDevice, VkRenderPass deferredPass, VkRenderPass forwardPass,
                                       float pointSize)
{
    deferredRenderer.init(vulkanDevice, deferredPass, pointSize);
    forwardRenderer.init(vulkanDevice, forwardPass, pointSize);
}

bool UniversalPointCloudRenderer::bindDeferred(vk::CommandBuffer cmd)
{
    return deferredRenderer.bind(cmd);
}

bool UniversalPointCloudRenderer::bindForward(vk::CommandBuffer cmd)
{
    return forwardRenderer.bind(cmd);
}

void UniversalPointCloudRenderer::pushModelDeferred(VkCommandBuffer cmd, mat4 model)
{
    deferredRenderer.pushModel(cmd, model);
}

void UniversalPointCloudRenderer::pushModelForward(VkCommandBuffer cmd, mat4 model)
{
    forwardRenderer.pushModel(cmd, model);
}

void UniversalPointCloudRenderer::updateUniformBuffersDeferred(vk::CommandBuffer cmd, mat4 view, mat4 proj)
{
    deferredRenderer.updateUniformBuffers(cmd, view, proj);
}

void UniversalPointCloudRenderer::updateUniformBuffersForward(vk::CommandBuffer cmd, mat4 view, mat4 proj)
{
    forwardRenderer.updateUniformBuffers(cmd, view, proj);
}

void UniversalPointCloudRenderer::reload()
{
    deferredRenderer.reload();
    forwardRenderer.reload();
}
}  // namespace Vulkan
}  // namespace Saiga
