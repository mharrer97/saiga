/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 *
 * Created by Mathias Harrer: mathias.mh.harrer@fau.de
 */

#pragma once

#include "saiga/core/camera/all.h"
#include "saiga/core/math/math.h"
#include "saiga/core/util/assert.h"
#include "saiga/vulkan/CommandPool.h"
#include "saiga/vulkan/FrameSync.h"
#include "saiga/vulkan/Queue.h"
#include "saiga/vulkan/Renderer.h"
#include "saiga/vulkan/VulkanAsset.h"
#include "saiga/vulkan/buffer/ColorBuffer.h"
#include "saiga/vulkan/buffer/DepthBuffer.h"
#include "saiga/vulkan/buffer/Framebuffer.h"
#include "saiga/vulkan/lighting/PointLight.h"
#include "saiga/vulkan/renderModules/QuadRenderer.h"
#include "saiga/vulkan/window/Window.h"



namespace Saiga
{
namespace Vulkan
{
class SAIGA_VULKAN_API VulkanDeferredRenderingInterface : public RenderingInterfaceBase
{
   public:
    VulkanDeferredRenderingInterface(RendererBase& parent) : RenderingInterfaceBase(parent) {}
    virtual ~VulkanDeferredRenderingInterface() {}

    virtual void transfer(vk::CommandBuffer cmd, Camera* cam) {}
    virtual void transferForward(vk::CommandBuffer cmd, Camera* cam) {}
    virtual void render(vk::CommandBuffer cmd, Camera* cam) {}
    virtual void renderForward(vk::CommandBuffer cmd, Camera* cam) {}
    virtual void renderGUI() {}
};


/*struct FrameBufferAttachment {
        VkImage image;
        VkDeviceMemory mem;
        VkImageView view;
        VkFormat format;
};*/


class SAIGA_VULKAN_API VulkanDeferredRenderer : public VulkanRenderer
{
   public:
    bool lightRotate     = true;
    float lightIntensity = 25.f;
    Saiga::Vulkan::Lighting::PointLight pointLight;

    CommandPool renderCommandPool;
    vk::RenderPass renderPass;
    vk::RenderPass lightingPass;
    vk::RenderPass forwardPass;

    VulkanDeferredRenderer(VulkanWindow& window, VulkanParameters vulkanParameters);
    virtual ~VulkanDeferredRenderer() override;

    virtual void render(FrameSync& sync, int currentImage, Camera* cam) override;
    //    virtual void render(Camera* cam) override;


    virtual void createBuffers(int numImages, int w, int h) override;
    //    void createAttachment(VkFormat format, VkImageUsageFlagBits usage, FrameBufferAttachment *attachment, int w,
    //    int h);

    virtual void reload();

    void setupRenderPass();

    void setupGeometryCommandBuffer(int currentImage, Camera* cam);

    void setupDrawCommandBuffer(int currentImage, Camera* cam);

    void setupForwardCommandBuffer(int currentImage, Camera* cam);

   protected:
    DepthBuffer depthBuffer;
    std::vector<vk::CommandBuffer> drawCmdBuffers;
    std::vector<vk::CommandBuffer> geometryCmdBuffers;
    std::vector<vk::CommandBuffer> forwardCmdBuffers;

    vk::Semaphore geometrySemaphore;
    vk::Semaphore deferredSemaphore;
    std::vector<Framebuffer> frameBuffers;
    Framebuffer gBuffer;

    // FrameBufferAttachment diffuseAttachment, specularAttachment, normalAttachment, additionalAttachment;
    ColorBuffer diffuseAttachment, specularAttachment, normalAttachment, additionalAttachment;
    DepthBuffer gBufferDepthBuffer;

    bool debug = false;

    // TODO test
    Saiga::Vulkan::QuadRenderer quadRenderer;
};


}  // namespace Vulkan
}  // namespace Saiga
