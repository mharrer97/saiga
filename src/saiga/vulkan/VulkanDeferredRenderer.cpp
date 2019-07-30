/*
 * Vulkan Example base class
 *
 * Copyright (C) 2016-2017 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */


#include "VulkanDeferredRenderer.h"

#include "saiga/vulkan/Shader/all.h"

#include "VulkanInitializers.hpp"

#if defined(SAIGA_OPENGL_INCLUDED)
#    error OpenGL was included somewhere.
#endif


namespace Saiga
{
namespace Vulkan
{
VulkanDeferredRenderer::VulkanDeferredRenderer(VulkanWindow& window, VulkanParameters vulkanParameters)
    : VulkanRenderer(window, vulkanParameters)
{
    setupRenderPass();
    renderCommandPool = base().mainQueue.createCommandPool(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    cout << "VulkanDeferredRenderer init done." << endl;
}

VulkanDeferredRenderer::~VulkanDeferredRenderer()
{
    base().device.destroyRenderPass(renderPass);
}


void VulkanDeferredRenderer::createBuffers(int numImages, int w, int h)
{
    depthBuffer.destroy();
    depthBuffer.init(base(), w, h);
    gBufferDepthBuffer.destroy();
    gBufferDepthBuffer.init(base(), w, h);

    frameBuffers.clear();
    frameBuffers.resize(numImages);
    for (int i = 0; i < numImages; i++)
    {
        frameBuffers[i].createColorDepthStencil(w, h, swapChain.buffers[i].view, depthBuffer.location->data.view,
                                                lightingPass, base().device);
    }

    //TODO use gbuffer correctly
    gBuffer.destroy();
    gBuffer.createGBuffer(w, h, (VkImageView)diffuseAttachment.view, (VkImageView)specularAttachment.view,
                          (VkImageView)normalAttachment.view, (VkImageView)additionalAttachment.view, gBufferDepthBuffer.location->data.view,
                          renderPass, base().device);

    renderCommandPool.freeCommandBuffers(drawCmdBuffers);
    drawCmdBuffers.clear();
    drawCmdBuffers = renderCommandPool.allocateCommandBuffers(numImages, vk::CommandBufferLevel::ePrimary);

    if (imGui) imGui->initResources(base(), renderPass);
}


// Create a frame buffer attachment
void VulkanDeferredRenderer::createAttachment(
    VkFormat format,
    VkImageUsageFlagBits usage,
    FrameBufferAttachment *attachment, int w, int h)
{
    /*VkImageAspectFlags aspectMask = 0;
    VkImageLayout imageLayout;

    attachment->format = format;

    if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
    {
        aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    assert(aspectMask > 0);

    VkImageCreateInfo image = vks::initializers::imageCreateInfo();
    image.imageType = VK_IMAGE_TYPE_2D;
    image.format = format;
    image.extent.width = w;
    image.extent.height = h;
    image.extent.depth = 1;
    image.mipLevels = 1;
    image.arrayLayers = 1;
    image.samples = VK_SAMPLE_COUNT_1_BIT;
    image.tiling = VK_IMAGE_TILING_OPTIMAL;
    image.usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT;

    VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
    VkMemoryRequirements memReqs;

    VK_CHECK_RESULT(vkCreateImage(base().device, &image, nullptr, &attachment->image));
    vkGetImageMemoryRequirements(base().device, attachment->image, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = base().device.getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(base().device, &memAlloc, nullptr, &attachment->mem));
    VK_CHECK_RESULT(vkBindImageMemory(base().device, attachment->image, attachment->mem, 0));

    VkImageViewCreateInfo imageView = vks::initializers::imageViewCreateInfo();
    imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageView.format = format;
    imageView.subresourceRange = {};
    imageView.subresourceRange.aspectMask = aspectMask;
    imageView.subresourceRange.baseMipLevel = 0;
    imageView.subresourceRange.levelCount = 1;
    imageView.subresourceRange.baseArrayLayer = 0;
    imageView.subresourceRange.layerCount = 1;
    imageView.image = attachment->image;
    VK_CHECK_RESULT(vkCreateImageView(base().device, &imageView, nullptr, &attachment->view));*/
}


void VulkanDeferredRenderer::setupRenderPass()
{

    // create gbuffer Pass
    std::array<VkAttachmentDescription, 5> gBufferAttachments = {};
    // Color attachment
    gBufferAttachments[0].format         = swapChain.colorFormat;
    gBufferAttachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
    gBufferAttachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    gBufferAttachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    gBufferAttachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    gBufferAttachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    gBufferAttachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    gBufferAttachments[0].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    // Depth attachment
    gBufferAttachments[1].format         = (VkFormat)depthBuffer.format;
    gBufferAttachments[1].samples        = VK_SAMPLE_COUNT_1_BIT;
    gBufferAttachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    gBufferAttachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    gBufferAttachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    gBufferAttachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    gBufferAttachments[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    gBufferAttachments[1].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference gBufferColorReference = {};
    gBufferColorReference.attachment            = 0;
    gBufferColorReference.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference gBufferDepthReference = {};
    gBufferDepthReference.attachment            = 1;
    gBufferDepthReference.layout                = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription gBufferSubpassDescription    = {};
    gBufferSubpassDescription.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    gBufferSubpassDescription.colorAttachmentCount    = 1;
    gBufferSubpassDescription.pColorAttachments       = &gBufferColorReference;
    gBufferSubpassDescription.pDepthStencilAttachment = &gBufferDepthReference;
    gBufferSubpassDescription.inputAttachmentCount    = 0;
    gBufferSubpassDescription.pInputAttachments       = nullptr;
    gBufferSubpassDescription.preserveAttachmentCount = 0;
    gBufferSubpassDescription.pPreserveAttachments    = nullptr;
    gBufferSubpassDescription.pResolveAttachments     = nullptr;

    // Subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 2> gBufferDependencies;

    gBufferDependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
    gBufferDependencies[0].dstSubpass      = 0;
    gBufferDependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    gBufferDependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    gBufferDependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
    gBufferDependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    gBufferDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    gBufferDependencies[1].srcSubpass      = 0;
    gBufferDependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
    gBufferDependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    gBufferDependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    gBufferDependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    gBufferDependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
    gBufferDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo gBufferRenderPassInfo = {};
    gBufferRenderPassInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    gBufferRenderPassInfo.attachmentCount        = 2;  // static_cast<uint32_t>(attachments.size());
    gBufferRenderPassInfo.pAttachments           = gBufferAttachments.data();
    gBufferRenderPassInfo.subpassCount           = 1;
    gBufferRenderPassInfo.pSubpasses             = &gBufferSubpassDescription;
    gBufferRenderPassInfo.dependencyCount        = 1;  // static_cast<uint32_t>(dependencies.size());
    gBufferRenderPassInfo.pDependencies          = gBufferDependencies.data();

    VK_CHECK_RESULT(vkCreateRenderPass(base().device, &gBufferRenderPassInfo, nullptr, &renderPass));

    //create lighting Pass

    std::array<VkAttachmentDescription, 2> attachments = {};
    // Color attachment
    attachments[0].format         = swapChain.colorFormat;
    attachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    // Depth attachment
    attachments[1].format         = (VkFormat)depthBuffer.format;
    attachments[1].samples        = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorReference = {};
    colorReference.attachment            = 0;
    colorReference.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthReference = {};
    depthReference.attachment            = 1;
    depthReference.layout                = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription    = {};
    subpassDescription.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount    = 1;
    subpassDescription.pColorAttachments       = &colorReference;
    subpassDescription.pDepthStencilAttachment = &depthReference;
    subpassDescription.inputAttachmentCount    = 0;
    subpassDescription.pInputAttachments       = nullptr;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments    = nullptr;
    subpassDescription.pResolveAttachments     = nullptr;

    // Subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 2> dependencies;

    dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass      = 0;
    dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass      = 0;
    dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount        = 2;  // static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments           = attachments.data();
    renderPassInfo.subpassCount           = 1;
    renderPassInfo.pSubpasses             = &subpassDescription;
    renderPassInfo.dependencyCount        = 1;  // static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies          = dependencies.data();

    VK_CHECK_RESULT(vkCreateRenderPass(base().device, &renderPassInfo, nullptr, &lightingPass));
}



void VulkanDeferredRenderer::render(FrameSync& sync, int currentImage)
{
    VulkanDeferredRenderingInterface* renderingInterface = dynamic_cast<VulkanDeferredRenderingInterface*>(rendering);
    SAIGA_ASSERT(renderingInterface);

    //    cout << "VulkanDeferredRenderer::render" << endl;
    if (imGui)
    {
        //        std::thread t([&](){
        imGui->beginFrame();
        renderingInterface->renderGUI();
        imGui->endFrame();
        //        });
        //        t.join();
    }



    VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
    //    cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    VkClearValue clearValues[2];

    // This is blender's default viewport background color :)
    vec4 clearColor             = vec4(57, 57, 57, 255) / 255.0f;
    clearValues[0].color        = {{clearColor[0], clearColor[1], clearColor[2], clearColor[3]}};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassBeginInfo    = vks::initializers::renderPassBeginInfo();
    renderPassBeginInfo.renderPass               = renderPass;
    renderPassBeginInfo.renderArea.offset.x      = 0;
    renderPassBeginInfo.renderArea.offset.y      = 0;
    renderPassBeginInfo.renderArea.extent.width  = surfaceWidth;
    renderPassBeginInfo.renderArea.extent.height = SurfaceHeight;
    renderPassBeginInfo.clearValueCount          = 2;
    renderPassBeginInfo.pClearValues             = clearValues;


    vk::CommandBuffer& cmd = drawCmdBuffers[currentImage];
    // cmd.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
    // Set target frame buffer
    renderPassBeginInfo.framebuffer = frameBuffers[currentImage].framebuffer;

    cmd.begin(cmdBufInfo);
    timings.resetFrame(cmd);
    timings.enterSection("TRANSFER", cmd);

    // VK_CHECK_RESULT(vkBeginCommandBuffer(cmd, &cmdBufInfo));
    renderingInterface->transfer(cmd);

    timings.leaveSection("TRANSFER", cmd);


    if (imGui) imGui->updateBuffers(cmd, currentImage);

    vkCmdBeginRenderPass(cmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = vks::initializers::viewport((float)surfaceWidth, (float)SurfaceHeight, 0.0f, 1.0f);
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = vks::initializers::rect2D(surfaceWidth, SurfaceHeight, 0, 0);
    vkCmdSetScissor(cmd, 0, 1, &scissor);


    {
        // Actual rendering
        timings.enterSection("MAIN", cmd);
        renderingInterface->render(cmd);
        timings.leaveSection("MAIN", cmd);
        timings.enterSection("IMGUI", cmd);
        if (imGui) imGui->render(cmd, currentImage);
        timings.leaveSection("IMGUI", cmd);
    }

    vkCmdEndRenderPass(cmd);


    VK_CHECK_RESULT(vkEndCommandBuffer(cmd));

    vk::PipelineStageFlags submitPipelineStages = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    std::array<vk::Semaphore, 2> signalSemaphores{sync.renderComplete, sync.defragMayStart};

    vk::SubmitInfo submitInfo;
    //    submitInfo = vks::initializers::submitInfo();
    submitInfo.pWaitDstStageMask    = &submitPipelineStages;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = &sync.imageAvailable;
    submitInfo.signalSemaphoreCount = 2;
    submitInfo.pSignalSemaphores    = signalSemaphores.data();

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &cmd;
    //    VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue, 1, &submitInfo, sync.frameFence));
    base().mainQueue.submit(submitInfo, sync.frameFence);

    timings.finishFrame(sync.defragMayStart);
    //    graphicsQueue.queue.submit(submitInfo,vk::Fence());

    //    VK_CHECK_RESULT(swapChain.queuePresent(presentQueue, currentBuffer,  sync.renderComplete));
    base().finish_frame();
}



}  // namespace Vulkan
}  // namespace Saiga
