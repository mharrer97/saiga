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
#include "saiga/vulkan/lighting/DeferredLighting.h"
#include "saiga/vulkan/lighting/PointLight.h"
#include "saiga/vulkan/raytracing/Raytracer.h"
#include "saiga/vulkan/raytracing/RaytracerGB.h"
#include "saiga/vulkan/renderModules/QuadRenderer.h"
#include "saiga/vulkan/window/Window.h"



namespace Saiga
{
namespace Vulkan
{
class SAIGA_VULKAN_API VulkanDeferredRenderingInterface : public RenderingInterfaceBase
{
   public:
    // VulkanDeferredRenderingInterface(RendererBase& parent) : RenderingInterfaceBase(parent) {}
    virtual ~VulkanDeferredRenderingInterface() {}

    virtual void transfer(vk::CommandBuffer cmd, Camera* cam) {}
    virtual void transferDepth(vk::CommandBuffer cmd, Camera* cam) {}
    virtual void transferForward(vk::CommandBuffer cmd, Camera* cam) {}
    virtual void render(vk::CommandBuffer cmd, Camera* cam) {}
    // render depth maps for shadow lights
    virtual void renderDepth(vk::CommandBuffer cmd, Camera* cam) {}
    virtual void renderForward(vk::CommandBuffer cmd, Camera* cam) {}
    virtual void renderGUI() {}
};


/*struct FrameBufferAttachment {
        VkImage image;
        VkDeviceMemory mem;
        VkImageView view;
        VkFormat format;
};*/

struct SAIGA_VULKAN_API DeferredRenderingParameters
{
    bool enableRTX = false;

    void fromConfigFile(const std::string& file);
};

class SAIGA_VULKAN_API VulkanDeferredRenderer : public VulkanRenderer
{
   public:
    using InterfaceType = VulkanDeferredRenderingInterface;
    using ParameterType = DeferredRenderingParameters;

    Saiga::Vulkan::Lighting::DeferredLighting lighting;
    Saiga::Vulkan::RTX::Raytracer raytracer;
    Saiga::Vulkan::RTX::Raytracer raytracerReflections;
    Saiga::Vulkan::RTX::RaytracerGB raytracerGB;


    CommandPool renderCommandPool;
    vk::RenderPass renderPass;
    vk::RenderPass lightingPass;
    vk::RenderPass forwardPass;

    VulkanDeferredRenderer(VulkanWindow& window, VulkanParameters vulkanParameters,
                           std::vector<std::string> additionalInstanceExtensions = {},
                           ParameterType rendererParameters                      = ParameterType());
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
    // std::shared_ptr<Lighting::DirectionalLight> directionalLight;

    ParameterType params;
    DepthBuffer depthBuffer;
    std::vector<vk::CommandBuffer> drawCmdBuffers;
    std::vector<vk::CommandBuffer> geometryCmdBuffers;
    std::vector<vk::CommandBuffer> forwardCmdBuffers;
    std::vector<vk::CommandBuffer> shadowCmdBuffers;
    std::vector<vk::CommandBuffer> RTXCmdBuffers;


    vk::Semaphore geometrySemaphore;
    vk::Semaphore shadowSemaphore;
    vk::Semaphore deferredSemaphore;
    vk::Semaphore RTXSemaphore;

    std::vector<Framebuffer> frameBuffers;
    Framebuffer gBuffer;
    Framebuffer rasterFramebuffer;

    // FrameBufferAttachment diffuseAttachment, specularAttachment, normalAttachment, additionalAttachment;
    ColorBuffer diffuseAttachment, specularAttachment, normalAttachment, additionalAttachment, rasterAttachment,
        RTXAttachment;
    DepthBuffer gBufferDepthBuffer, rast;

    bool debug                    = false;
    bool lightDebug               = false;
    bool renderLights             = true;
    bool showRTX                  = true;
    bool rtxRenderModeReflections = false;
    bool hybridRendering          = true;
};


}  // namespace Vulkan
}  // namespace Saiga
