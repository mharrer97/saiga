#include "DeferredRenderer.h"

#if defined(SAIGA_OPENGL_INCLUDED)
#    error OpenGL was included somewhere.
#endif

namespace Saiga
{
namespace Vulkan
{
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
}  // namespace Vulkan
}  // namespace Saiga
