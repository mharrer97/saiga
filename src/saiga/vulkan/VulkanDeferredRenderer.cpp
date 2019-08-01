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
    setupColorAttachmentSampler();
    renderCommandPool = base().mainQueue.createCommandPool(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    cout << "VulkanDeferredRenderer init done." << endl;
}

VulkanDeferredRenderer::~VulkanDeferredRenderer()
{
    base().device.destroyRenderPass(renderPass);
}

//!
//! \brief VulkanDeferredRenderer::createBuffers creates all framebuffers and attachments for deferred rendering
//! \param numImages
//! \param w
//! \param h
//!
void VulkanDeferredRenderer::createBuffers(int numImages, int w, int h)
{
    depthBuffer.destroy();
    depthBuffer.init(base(), w, h);

    //gbuffer and attachments
    gBufferDepthBuffer.destroy();
    gBufferDepthBuffer.init(base(), w, h);

    diffuseAttachment.destroy();
    specularAttachment.destroy();
    normalAttachment.destroy();
    additionalAttachment.destroy();

    diffuseAttachment.init(base(), w, h, vk::ImageUsageFlagBits::eSampled);
    specularAttachment.init(base(), w, h, vk::ImageUsageFlagBits::eSampled);
    normalAttachment.init(base(), w, h, vk::ImageUsageFlagBits::eSampled);
    additionalAttachment.init(base(), w, h, vk::ImageUsageFlagBits::eSampled);

    frameBuffers.clear();
    frameBuffers.resize(numImages);
    for (int i = 0; i < numImages; i++)
    {
        frameBuffers[i].createColorDepthStencil(w, h, swapChain.buffers[i].view, depthBuffer.location->data.view,
                                                lightingPass, base().device);
    }

    //TODO use gbuffer correctly
    gBuffer.destroy();
    gBuffer.createGBuffer(w, h, diffuseAttachment.location->data.view, specularAttachment.location->data.view,
                          normalAttachment.location->data.view, additionalAttachment.location->data.view,
                          gBufferDepthBuffer.location->data.view, renderPass, base().device);

    renderCommandPool.freeCommandBuffers(drawCmdBuffers);
    drawCmdBuffers.clear();
    drawCmdBuffers = renderCommandPool.allocateCommandBuffers(numImages, vk::CommandBufferLevel::ePrimary);

    renderCommandPool.freeCommandBuffer(geometryCmdBuffer);
    geometryCmdBuffer = renderCommandPool.allocateCommandBuffer(vk::CommandBufferLevel::ePrimary);

    if (imGui) imGui->initResources(base(), renderPass);
}


//!
//! \brief VulkanDeferredRenderer::setupRenderPass
//!
//! sets up the render passes for deferred rendering
//!
void VulkanDeferredRenderer::setupRenderPass()
{

    // create gbuffer Pass
    std::array<vk::AttachmentDescription, 5> gBufferAttachments = {};
    // diffuse attachment
    gBufferAttachments[0].format         = diffuseAttachment.format;
    gBufferAttachments[0].samples        = vk::SampleCountFlagBits::e1;
    gBufferAttachments[0].loadOp         = vk::AttachmentLoadOp::eClear;
    gBufferAttachments[0].storeOp        = vk::AttachmentStoreOp::eStore;
    gBufferAttachments[0].stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
    gBufferAttachments[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    gBufferAttachments[0].initialLayout  = vk::ImageLayout::eUndefined;
    gBufferAttachments[0].finalLayout    = vk::ImageLayout::eShaderReadOnlyOptimal;
    // specular attachment
    gBufferAttachments[1].format         = specularAttachment.format;
    gBufferAttachments[1].samples        = vk::SampleCountFlagBits::e1;
    gBufferAttachments[1].loadOp         = vk::AttachmentLoadOp::eClear;
    gBufferAttachments[1].storeOp        = vk::AttachmentStoreOp::eStore;
    gBufferAttachments[1].stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
    gBufferAttachments[1].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    gBufferAttachments[1].initialLayout  = vk::ImageLayout::eUndefined;
    gBufferAttachments[1].finalLayout    = vk::ImageLayout::eShaderReadOnlyOptimal;
    // normalattachment
    gBufferAttachments[2].format         = normalAttachment.format;
    gBufferAttachments[2].samples        = vk::SampleCountFlagBits::e1;
    gBufferAttachments[2].loadOp         = vk::AttachmentLoadOp::eClear;
    gBufferAttachments[2].storeOp        = vk::AttachmentStoreOp::eStore;
    gBufferAttachments[2].stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
    gBufferAttachments[2].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    gBufferAttachments[2].initialLayout  = vk::ImageLayout::eUndefined;
    gBufferAttachments[2].finalLayout    = vk::ImageLayout::eShaderReadOnlyOptimal;
    // additionalattachment
    gBufferAttachments[3].format         = additionalAttachment.format;
    gBufferAttachments[3].samples        = vk::SampleCountFlagBits::e1;
    gBufferAttachments[3].loadOp         = vk::AttachmentLoadOp::eClear;
    gBufferAttachments[3].storeOp        = vk::AttachmentStoreOp::eStore;
    gBufferAttachments[3].stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
    gBufferAttachments[3].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    gBufferAttachments[3].initialLayout  = vk::ImageLayout::eUndefined;
    gBufferAttachments[3].finalLayout    = vk::ImageLayout::eShaderReadOnlyOptimal;
    // Depth attachment
    gBufferAttachments[4].format         = gBufferDepthBuffer.format;
    gBufferAttachments[4].samples        = vk::SampleCountFlagBits::e1;
    gBufferAttachments[4].loadOp         = vk::AttachmentLoadOp::eClear;
    gBufferAttachments[4].storeOp        = vk::AttachmentStoreOp::eStore;
    gBufferAttachments[4].stencilLoadOp  = vk::AttachmentLoadOp::eClear;
    gBufferAttachments[4].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    gBufferAttachments[4].initialLayout  = vk::ImageLayout::eUndefined;
    gBufferAttachments[4].finalLayout    = vk::ImageLayout::eShaderReadOnlyOptimal;

    std::vector<vk::AttachmentReference> gBufferColorReferences = {};
    gBufferColorReferences.push_back({0, vk::ImageLayout::eColorAttachmentOptimal});
    gBufferColorReferences.push_back({1, vk::ImageLayout::eColorAttachmentOptimal});
    gBufferColorReferences.push_back({2, vk::ImageLayout::eColorAttachmentOptimal});
    gBufferColorReferences.push_back({3, vk::ImageLayout::eColorAttachmentOptimal});

    vk::AttachmentReference gBufferDepthReference = {};
    gBufferDepthReference.attachment            = 4;
    gBufferDepthReference.layout                = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::SubpassDescription gBufferSubpassDescription    = {};
    gBufferSubpassDescription.pipelineBindPoint       = vk::PipelineBindPoint::eGraphics;
    gBufferSubpassDescription.colorAttachmentCount    = static_cast<uint32_t>(gBufferColorReferences.size());
    gBufferSubpassDescription.pColorAttachments       = gBufferColorReferences.data();
    gBufferSubpassDescription.pDepthStencilAttachment = &gBufferDepthReference;
    gBufferSubpassDescription.inputAttachmentCount    = 0;
    gBufferSubpassDescription.pInputAttachments       = nullptr;
    gBufferSubpassDescription.preserveAttachmentCount = 0;
    gBufferSubpassDescription.pPreserveAttachments    = nullptr;
    gBufferSubpassDescription.pResolveAttachments     = nullptr;

    // Subpass dependencies for layout transitions
    std::array<vk::SubpassDependency, 2> gBufferDependencies;

    gBufferDependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
    gBufferDependencies[0].dstSubpass      = 0;
    gBufferDependencies[0].srcStageMask    = vk::PipelineStageFlagBits::eBottomOfPipe;
    gBufferDependencies[0].dstStageMask    = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    gBufferDependencies[0].srcAccessMask   = vk::AccessFlagBits::eMemoryRead;
    gBufferDependencies[0].dstAccessMask   = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
    gBufferDependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

    gBufferDependencies[1].srcSubpass      = 0;
    gBufferDependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
    gBufferDependencies[1].srcStageMask    = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    gBufferDependencies[1].dstStageMask    = vk::PipelineStageFlagBits::eBottomOfPipe;
    gBufferDependencies[1].srcAccessMask   = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
    gBufferDependencies[1].dstAccessMask   = vk::AccessFlagBits::eMemoryRead;
    gBufferDependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

    vk::RenderPassCreateInfo gBufferRenderPassInfo = {};
    //gBufferRenderPassInfo.sType                  = vk::StructureType::eRenderPassCreateInfo;
    gBufferRenderPassInfo.attachmentCount        = static_cast<uint32_t>(gBufferAttachments.size());
    gBufferRenderPassInfo.pAttachments           = gBufferAttachments.data();
    gBufferRenderPassInfo.subpassCount           = 1;
    gBufferRenderPassInfo.pSubpasses             = &gBufferSubpassDescription;
    gBufferRenderPassInfo.dependencyCount        = static_cast<uint32_t>(gBufferDependencies.size());
    gBufferRenderPassInfo.pDependencies          = gBufferDependencies.data();

    //VK_CHECK_RESULT(vkCreateRenderPass(base().device, &gBufferRenderPassInfo, nullptr, &renderPass));
    base().device.createRenderPass(&gBufferRenderPassInfo, nullptr, &renderPass);
    SAIGA_ASSERT(renderPass);


    //create lighting Pass

    std::array<vk::AttachmentDescription, 2> attachments = {};
    // Color attachment
    attachments[0].format         = (vk::Format)swapChain.colorFormat;
    attachments[0].samples        = vk::SampleCountFlagBits::e1;
    attachments[0].loadOp         = vk::AttachmentLoadOp::eClear;
    attachments[0].storeOp        = vk::AttachmentStoreOp::eStore;
    attachments[0].stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
    attachments[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    attachments[0].initialLayout  = vk::ImageLayout::eUndefined;
    attachments[0].finalLayout    = vk::ImageLayout::ePresentSrcKHR;
    // Depth attachment
    attachments[1].format         = depthBuffer.format;
    attachments[1].samples        = vk::SampleCountFlagBits::e1;
    attachments[1].loadOp         = vk::AttachmentLoadOp::eClear;
    attachments[1].storeOp        = vk::AttachmentStoreOp::eStore;
    attachments[1].stencilLoadOp  = vk::AttachmentLoadOp::eClear;
    attachments[1].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    attachments[1].initialLayout  = vk::ImageLayout::eUndefined;
    attachments[1].finalLayout    = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::AttachmentReference colorReference = {};
    colorReference.attachment            = 0;
    colorReference.layout                = vk::ImageLayout::eColorAttachmentOptimal;

    vk::AttachmentReference depthReference = {};
    depthReference.attachment            = 1;
    depthReference.layout                = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::SubpassDescription subpassDescription    = {};
    subpassDescription.pipelineBindPoint       = vk::PipelineBindPoint::eGraphics;
    subpassDescription.colorAttachmentCount    = 1;
    subpassDescription.pColorAttachments       = &colorReference;
    subpassDescription.pDepthStencilAttachment = &depthReference;
    subpassDescription.inputAttachmentCount    = 0;
    subpassDescription.pInputAttachments       = nullptr;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments    = nullptr;
    subpassDescription.pResolveAttachments     = nullptr;

    // Subpass dependencies for layout transitions
    std::array<vk::SubpassDependency, 2> dependencies;

    dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass      = 0;
    dependencies[0].srcStageMask    = vk::PipelineStageFlagBits::eBottomOfPipe;
    dependencies[0].dstStageMask    = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependencies[0].srcAccessMask   = vk::AccessFlagBits::eMemoryRead;
    dependencies[0].dstAccessMask   = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
    dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

    dependencies[1].srcSubpass      = 0;
    dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask    = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependencies[1].dstStageMask    = vk::PipelineStageFlagBits::eBottomOfPipe;
    dependencies[1].srcAccessMask   = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
    dependencies[1].dstAccessMask   = vk::AccessFlagBits::eMemoryRead;
    dependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

    vk::RenderPassCreateInfo renderPassInfo = {};
    //renderPassInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount        = 2;  // static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments           = attachments.data();
    renderPassInfo.subpassCount           = 1;
    renderPassInfo.pSubpasses             = &subpassDescription;
    renderPassInfo.dependencyCount        = 1;  // static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies          = dependencies.data();

    base().device.createRenderPass(&renderPassInfo, nullptr, &lightingPass);
    SAIGA_ASSERT(lightingPass);}

//!
//! \brief VulkanDeferredRenderer::setupColorAttachmentSampler
//!
//! creates a sampler to sample from the color attachments of the geometry pass
//!
void VulkanDeferredRenderer::setupColorAttachmentSampler(){

    vk::SamplerCreateInfo sampler = {};
    sampler.magFilter = vk::Filter::eNearest;
    sampler.minFilter = vk::Filter::eNearest;
    sampler.mipmapMode = vk::SamplerMipmapMode::eLinear;
    sampler.addressModeU = vk::SamplerAddressMode::eClampToEdge;
    sampler.addressModeV = sampler.addressModeU;
    sampler.addressModeW = sampler.addressModeU;
    sampler.mipLodBias = 0.0f;
    sampler.maxAnisotropy = 1.0f;
    sampler.minLod = 0.0f;
    sampler.maxLod = 1.0f;
    sampler.borderColor = vk::BorderColor::eFloatOpaqueWhite;

    base().device.createSampler(&sampler, nullptr, &colorSampler);
    SAIGA_ASSERT(colorSampler);
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


    vk::CommandBufferBeginInfo geometryCmdBufInfo = vks::initializers::commandBufferBeginInfo();
    //    cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    vk::ClearValue geometryClearValues[2];

    // This is blender's default viewport background color :)
    vec4 geometryClearColor             = vec4(57, 57, 57, 255) / 255.0f;
    geometryClearValues[0].color        = {{geometryClearColor[0], geometryClearColor[1], geometryClearColor[2], geometryClearColor[3]}};
    geometryClearValues[1].depthStencil = {1.0f, 0};

    vk::RenderPassBeginInfo geometryRenderPassBeginInfo    = vks::initializers::renderPassBeginInfo();
    geometryRenderPassBeginInfo.renderPass               = renderPass;
    geometryRenderPassBeginInfo.renderArea.offset.x      = 0;
    geometryRenderPassBeginInfo.renderArea.offset.y      = 0;
    geometryRenderPassBeginInfo.renderArea.extent.width  = surfaceWidth;
    geometryRenderPassBeginInfo.renderArea.extent.height = SurfaceHeight;
    geometryRenderPassBeginInfo.clearValueCount          = 2;
    geometryRenderPassBeginInfo.pClearValues             = clearValues;


    vk::CommandBuffer& cmd = geometryCmdBuffer;
    // cmd.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
    // Set target frame buffer
    renderPassBeginInfo.framebuffer = gBuffer;

    cmd.begin(cmdBufInfo);
    timings.resetFrame(cmd);
    timings.enterSection("TRANSFER", cmd);

    // VK_CHECK_RESULT(vkBeginCommandBuffer(cmd, &cmdBufInfo));
    renderingInterface->transfer(cmd);

    timings.leaveSection("TRANSFER", cmd);


    if (imGui) imGui->updateBuffers(cmd, currentImage);

    cmd.beginRenderPass( &renderPassBeginInfo, vk::SubpassContents::eInline);

    vk::Viewport gBufferViewport = vks::initializers::viewport((float)surfaceWidth, (float)SurfaceHeight, 0.0f, 1.0f);
    cmd.setViewport(0, 1, &gBufferViewport);
    vk::Rect2D gBufferScissor = vks::initializers::rect2D(surfaceWidth, SurfaceHeight, 0, 0);
    cmd.setScissor( 0, 1, &gBufferScissor);


    {
        // Actual rendering
        timings.enterSection("MAIN", cmd);
        renderingInterface->render(cmd);
        timings.leaveSection("MAIN", cmd);
        timings.enterSection("IMGUI", cmd);
        if (imGui) imGui->render(cmd, currentImage);
        timings.leaveSection("IMGUI", cmd);
    }

    cmd.endRenderPass();


    //VK_CHECK_RESULT(vkEndCommandBuffer(cmd));
    cmd.end();

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


    //TODO:
    // create descriptor sets for lighting pass -> use the sampler
    // make cmd buffer to render a quad
    // change everything to compute shader
    //TODO

/*
    //lighting pass

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


    //if (imGui) imGui->updateBuffers(cmd, currentImage);

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

    */
}



}  // namespace Vulkan
}  // namespace Saiga
