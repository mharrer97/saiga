/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 *
 * Created by Mathias Harrer: mathias.mh.harrer@fau.de
 */

#include "VulkanDeferredRenderer.h"

#include "saiga/core/util/ini/ini.h"
#include "saiga/vulkan/Shader/all.h"

#include "VulkanInitializers.hpp"


#if defined(SAIGA_OPENGL_INCLUDED)
#    error OpenGL was included somewhere.
#endif


namespace Saiga
{
namespace Vulkan
{
void DeferredRenderingParameters::fromConfigFile(const std::string& file)
{
    Saiga::SimpleIni ini;
    ini.LoadFile(file.c_str());

    enableRTX = ini.GetAddBool("Renderer", "enableRTX", false);
    if (ini.changed()) ini.SaveFile(file.c_str());
}
VulkanDeferredRenderer::VulkanDeferredRenderer(VulkanWindow& window, VulkanParameters vulkanParameters,
                                               std::vector<std::string> additionalInstanceExtensions,
                                               ParameterType rendererParameters)
    : VulkanRenderer(window, vulkanParameters, additionalInstanceExtensions),
      lighting(),
      raytracer(),
      params(rendererParameters)
{
    std::cout << "VulkanDeferredRenderer Creation -- START" << std::endl;

    setupRenderPass();
    // setupColorAttachmentSampler();
    std::cout << "RenderPass Creation -- FINISHED" << std::endl;

    // init deferred lighting

    lighting.init(base(), lightingPass);


    /*directionalLight = lighting.createDirectionalLight();
    directionalLight->setColorDiffuse(Saiga::Vulkan::Lighting::LightColorPresets::MoonlightBlue);
    directionalLight->setColorSpecular(Saiga::Vulkan::Lighting::LightColorPresets::MoonlightBlue);
    // directionalLight->setView(vec3(1.f, 1.f, 1.f), vec3(0.f, 0.f, 0.f), vec3(0.f, 1.f, 0.f));
    directionalLight->setDirection(vec3(-1.f, -1.f, -1.f));
    directionalLight->calculateModel();

    lighting.enableShadowMapping(directionalLight);*/


    // create semaphore for synchronization (offscreen rendering nad gbuffer usage)
    vk::SemaphoreCreateInfo semCreateInfo = vks::initializers::semaphoreCreateInfo();
    base().device.createSemaphore(&semCreateInfo, nullptr, &geometrySemaphore);
    // create Semaphore to signal, the shadowpass has finished completely
    vk::SemaphoreCreateInfo shadowSemCreateInfo = vks::initializers::semaphoreCreateInfo();
    base().device.createSemaphore(&shadowSemCreateInfo, nullptr, &shadowSemaphore);
    // create Semaphore to signal, the deferred pass has finished completely
    vk::SemaphoreCreateInfo deferredSemCreateInfo = vks::initializers::semaphoreCreateInfo();
    base().device.createSemaphore(&deferredSemCreateInfo, nullptr, &deferredSemaphore);
    // create Semaphore to signal, the rtx pass has finished completely
    vk::SemaphoreCreateInfo RTXSemCreateInfo = vks::initializers::semaphoreCreateInfo();
    base().device.createSemaphore(&RTXSemCreateInfo, nullptr, &RTXSemaphore);

    renderCommandPool = base().mainQueue.createCommandPool(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    std::cout << "VulkanDeferredRenderer init done." << std::endl;
    std::cout << "VulkanDeferredRenderer Creation -- FINISHED" << std::endl;
}

VulkanDeferredRenderer::~VulkanDeferredRenderer()
{
    std::cout << "Destroying VulkanDeferredRenderer" << std::endl;

    base().device.destroySemaphore(geometrySemaphore);
    base().device.destroySemaphore(shadowSemaphore);
    base().device.destroySemaphore(deferredSemaphore);
    base().device.destroySemaphore(RTXSemaphore);
    base().device.destroyRenderPass(renderPass);
    base().device.destroyRenderPass(lightingPass);
    base().device.destroyRenderPass(forwardPass);
}

//!
//! \brief VulkanDeferredRenderer::createBuffers creates all framebuffers and attachments for deferred rendering
//! \param numImages number of images in the swap chain
//! \param w width of the swap chain
//! \param h height of the swap chain
//!
void VulkanDeferredRenderer::createBuffers(int numImages, int w, int h)
{
    std::cout << "Buffer Creation -- START" << std::endl;

    depthBuffer.destroy();
    depthBuffer.init(base(), w, h);

    std::cout << "  DepthBuffer Creation -- FINISHED" << std::endl;

    // gbuffer and attachments
    gBufferDepthBuffer.destroy();
    gBufferDepthBuffer.init(base(), w, h, true);

    std::cout << "  GBufferDepthBufferCreation -- FINISHED" << std::endl;

    diffuseAttachment.destroy();
    specularAttachment.destroy();
    normalAttachment.destroy();
    additionalAttachment.destroy();

    diffuseAttachment.init(base(), w, h, vk::ImageUsageFlagBits::eSampled);
    specularAttachment.init(base(), w, h, vk::ImageUsageFlagBits::eSampled);
    normalAttachment.init(base(), w, h, vk::ImageUsageFlagBits::eSampled);
    additionalAttachment.init(base(), w, h, vk::ImageUsageFlagBits::eSampled);

    std::cout << "  Attachment Creation -- FINISHED" << std::endl;

    frameBuffers.clear();
    frameBuffers.resize(numImages);
    for (int i = 0; i < numImages; i++)
    {
        frameBuffers[i].createColorDepthStencil(w, h, swapChain.buffers[i].view, depthBuffer.location->data.view,
                                                lightingPass, base().device);
    }

    std::cout << "  Framebuffer Creation -- FINISHED" << std::endl;

    gBuffer.destroy();
    gBuffer.createGBuffer(w, h, diffuseAttachment.location->data.view, specularAttachment.location->data.view,
                          normalAttachment.location->data.view, additionalAttachment.location->data.view,
                          gBufferDepthBuffer.location->data.view, renderPass, base().device);

    std::cout << "  GBuffer Creation -- FINISHED" << std::endl;

    renderCommandPool.freeCommandBuffers(drawCmdBuffers);
    drawCmdBuffers.clear();
    drawCmdBuffers = renderCommandPool.allocateCommandBuffers(numImages, vk::CommandBufferLevel::ePrimary);

    std::cout << "  Command Buffer Allocation -- FINISHED" << std::endl;

    renderCommandPool.freeCommandBuffers(geometryCmdBuffers);
    geometryCmdBuffers = renderCommandPool.allocateCommandBuffers(numImages, vk::CommandBufferLevel::ePrimary);

    std::cout << "  Geometry Command Buffer Allocation -- FINISHED" << std::endl;

    renderCommandPool.freeCommandBuffers(forwardCmdBuffers);
    forwardCmdBuffers = renderCommandPool.allocateCommandBuffers(numImages, vk::CommandBufferLevel::ePrimary);
    std::cout << "  Forward Command Buffer Allocation -- FINISHED" << std::endl;

    renderCommandPool.freeCommandBuffers(shadowCmdBuffers);
    shadowCmdBuffers = renderCommandPool.allocateCommandBuffers(numImages, vk::CommandBufferLevel::ePrimary);
    std::cout << "  ShadowCommand Buffer Allocation -- FINISHED" << std::endl;

    renderCommandPool.freeCommandBuffers(RTXCmdBuffers);
    RTXCmdBuffers = renderCommandPool.allocateCommandBuffers(numImages, vk::CommandBufferLevel::ePrimary);
    std::cout << "  RTXCommand Buffer Allocation -- FINISHED" << std::endl;

    if (imGui) imGui->initResources(base(), forwardPass);

    std::cout << "QuadRenderer DescriptorSet Update/Creation -- CALL" << std::endl;

    lighting.createAndUpdateDescriptorSets(diffuseAttachment.location, specularAttachment.location,
                                           normalAttachment.location, additionalAttachment.location,
                                           gBufferDepthBuffer.location);
    std::cout << "QuadRenderer DescriptorSet Update/Creation -- CALL RETURN" << std::endl;

    // if RTX is enabled, destroy and create rtx components as well
    if (params.enableRTX)
    {
        std::cout << "Raytracer Creation -- CALL" << std::endl;
        // raytracer.destroy();
        raytracer.init(base(), vk::Format::eB8G8R8A8Unorm, w, h);
        std::cout << "Raytracer Creation -- FINISHED" << std::endl;
    }
    std::cout << "Buffer Creation -- FINISHED" << std::endl;
}

void VulkanDeferredRenderer::reload()
{
    lighting.reload();
}

//!
//! \brief VulkanDeferredRenderer::setupRenderPass
//!
//! sets up the render passes for deferred rendering and forward rendering
//!
void VulkanDeferredRenderer::setupRenderPass()
{
    std::cout << "Creation Render Passes -- START" << std::endl;
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
    gBufferDepthReference.attachment              = 4;
    gBufferDepthReference.layout                  = vk::ImageLayout::eDepthStencilAttachmentOptimal;


    vk::SubpassDescription gBufferSubpassDescription  = {};
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

    gBufferDependencies[0].srcSubpass    = VK_SUBPASS_EXTERNAL;
    gBufferDependencies[0].dstSubpass    = 0;
    gBufferDependencies[0].srcStageMask  = vk::PipelineStageFlagBits::eBottomOfPipe;
    gBufferDependencies[0].dstStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    gBufferDependencies[0].srcAccessMask = vk::AccessFlagBits::eMemoryRead;
    gBufferDependencies[0].dstAccessMask =
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
    gBufferDependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

    gBufferDependencies[1].srcSubpass   = 0;
    gBufferDependencies[1].dstSubpass   = VK_SUBPASS_EXTERNAL;
    gBufferDependencies[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    gBufferDependencies[1].dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
    gBufferDependencies[1].srcAccessMask =
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
    gBufferDependencies[1].dstAccessMask   = vk::AccessFlagBits::eMemoryRead;
    gBufferDependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

    vk::RenderPassCreateInfo gBufferRenderPassInfo = {};
    gBufferRenderPassInfo.attachmentCount          = static_cast<uint32_t>(gBufferAttachments.size());
    gBufferRenderPassInfo.pAttachments             = gBufferAttachments.data();
    gBufferRenderPassInfo.subpassCount             = 1;
    gBufferRenderPassInfo.pSubpasses               = &gBufferSubpassDescription;
    gBufferRenderPassInfo.dependencyCount          = static_cast<uint32_t>(gBufferDependencies.size());
    gBufferRenderPassInfo.pDependencies            = gBufferDependencies.data();

    base().device.createRenderPass(&gBufferRenderPassInfo, nullptr, &renderPass);
    SAIGA_ASSERT(renderPass);
    std::cout << "Creation Render Pass 1 -- FINISHED" << std::endl;


    // create lighting Pass

    std::array<vk::AttachmentDescription, 2> attachments = {};
    // Color attachment
    attachments[0].format         = (vk::Format)swapChain.colorFormat;
    attachments[0].samples        = vk::SampleCountFlagBits::e1;
    attachments[0].loadOp         = vk::AttachmentLoadOp::eClear;
    attachments[0].storeOp        = vk::AttachmentStoreOp::eStore;
    attachments[0].stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
    attachments[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    attachments[0].initialLayout  = vk::ImageLayout::eUndefined;
    attachments[0].finalLayout    = vk::ImageLayout::eColorAttachmentOptimal;
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
    colorReference.attachment              = 0;
    colorReference.layout                  = vk::ImageLayout::eColorAttachmentOptimal;

    vk::AttachmentReference depthReference = {};
    depthReference.attachment              = 1;
    depthReference.layout                  = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::SubpassDescription subpassDescription  = {};
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

    dependencies[0].srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass    = 0;
    dependencies[0].srcStageMask  = vk::PipelineStageFlagBits::eBottomOfPipe;
    dependencies[0].dstStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependencies[0].srcAccessMask = vk::AccessFlagBits::eMemoryRead;
    dependencies[0].dstAccessMask =
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
    dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

    dependencies[1].srcSubpass   = 0;
    dependencies[1].dstSubpass   = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependencies[1].dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
    dependencies[1].srcAccessMask =
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
    dependencies[1].dstAccessMask   = vk::AccessFlagBits::eMemoryRead;
    dependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

    vk::RenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.attachmentCount          = 2;  // static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments             = attachments.data();
    renderPassInfo.subpassCount             = 1;
    renderPassInfo.pSubpasses               = &subpassDescription;
    renderPassInfo.dependencyCount          = 1;  // static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies            = dependencies.data();

    base().device.createRenderPass(&renderPassInfo, nullptr, &lightingPass);
    SAIGA_ASSERT(lightingPass);

    std::cout << "Creation Render Pass 2 -- FINISHED" << std::endl;



    // create forward render pass
    std::array<vk::AttachmentDescription, 2> forwardAttachments = {};
    // Color attachment
    forwardAttachments[0].format         = (vk::Format)swapChain.colorFormat;
    forwardAttachments[0].samples        = vk::SampleCountFlagBits::e1;
    forwardAttachments[0].loadOp         = vk::AttachmentLoadOp::eLoad;
    forwardAttachments[0].storeOp        = vk::AttachmentStoreOp::eStore;
    forwardAttachments[0].stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
    forwardAttachments[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    forwardAttachments[0].initialLayout  = vk::ImageLayout::eColorAttachmentOptimal;
    forwardAttachments[0].finalLayout    = vk::ImageLayout::ePresentSrcKHR;
    // Depth attachment
    forwardAttachments[1].format         = (vk::Format)depthBuffer.format;
    forwardAttachments[1].samples        = vk::SampleCountFlagBits::e1;
    forwardAttachments[1].loadOp         = vk::AttachmentLoadOp::eLoad;
    forwardAttachments[1].storeOp        = vk::AttachmentStoreOp::eStore;
    forwardAttachments[1].stencilLoadOp  = vk::AttachmentLoadOp::eLoad;
    forwardAttachments[1].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    forwardAttachments[1].initialLayout  = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    forwardAttachments[1].finalLayout    = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::AttachmentReference forwardColorReference = {};
    forwardColorReference.attachment              = 0;
    forwardColorReference.layout                  = vk::ImageLayout::eColorAttachmentOptimal;

    vk::AttachmentReference forwardDepthReference = {};
    forwardDepthReference.attachment              = 1;
    forwardDepthReference.layout                  = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::SubpassDescription forwardSubpassDescription  = {};
    forwardSubpassDescription.pipelineBindPoint       = vk::PipelineBindPoint::eGraphics;
    forwardSubpassDescription.colorAttachmentCount    = 1;
    forwardSubpassDescription.pColorAttachments       = &forwardColorReference;
    forwardSubpassDescription.pDepthStencilAttachment = &forwardDepthReference;
    forwardSubpassDescription.inputAttachmentCount    = 0;
    forwardSubpassDescription.pInputAttachments       = nullptr;
    forwardSubpassDescription.preserveAttachmentCount = 0;
    forwardSubpassDescription.pPreserveAttachments    = nullptr;
    forwardSubpassDescription.pResolveAttachments     = nullptr;

    // Subpass dependencies for layout transitions
    std::array<vk::SubpassDependency, 2> forwardDependencies;

    forwardDependencies[0].srcSubpass    = VK_SUBPASS_EXTERNAL;
    forwardDependencies[0].dstSubpass    = 0;
    forwardDependencies[0].srcStageMask  = vk::PipelineStageFlagBits::eBottomOfPipe;
    forwardDependencies[0].dstStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    forwardDependencies[0].srcAccessMask = vk::AccessFlagBits::eMemoryRead;
    forwardDependencies[0].dstAccessMask =
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
    forwardDependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

    forwardDependencies[1].srcSubpass   = 0;
    forwardDependencies[1].dstSubpass   = VK_SUBPASS_EXTERNAL;
    forwardDependencies[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    forwardDependencies[1].dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
    forwardDependencies[1].srcAccessMask =
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
    forwardDependencies[1].dstAccessMask = vk::AccessFlagBits::eMemoryRead;
    ;
    forwardDependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

    vk::RenderPassCreateInfo forwardRenderPassInfo = {};
    forwardRenderPassInfo.attachmentCount          = 2;  // static_cast<uint32_t>(attachments.size());
    forwardRenderPassInfo.pAttachments             = forwardAttachments.data();
    forwardRenderPassInfo.subpassCount             = 1;
    forwardRenderPassInfo.pSubpasses               = &forwardSubpassDescription;
    forwardRenderPassInfo.dependencyCount          = 1;  // static_cast<uint32_t>(dependencies.size());
    forwardRenderPassInfo.pDependencies            = forwardDependencies.data();

    base().device.createRenderPass(&forwardRenderPassInfo, nullptr, &forwardPass);
    SAIGA_ASSERT(forwardPass);
    std::cout << "Creation Render Pass 3 -- FINISHED" << std::endl;
    std::cout << "Creation Render Passes -- FINISHED" << std::endl;
}

//!
//! \brief VulkanDeferredRenderer::setupGeometryCommandBuffer
//! \param currentImage indicates, which which cmdbuffer should be recorded
//! \param cam the camera to be used while rendering
//!
//! creates the command buffers for rendering the scene into the gbuffer
//!
void VulkanDeferredRenderer::setupGeometryCommandBuffer(int currentImage, Camera* cam)
{
    VulkanDeferredRenderingInterface* renderingInterface = dynamic_cast<VulkanDeferredRenderingInterface*>(rendering);
    SAIGA_ASSERT(renderingInterface);

    // create and fill infos for cmd buffer recording
    vk::CommandBufferBeginInfo geometryCmdBufInfo = vks::initializers::commandBufferBeginInfo();

    std::array<vk::ClearValue, 5> geometryClearValues;

    // This is blender's default viewport background color :)
    vec4 clearColor = vec4(57, 57, 57, 255) / 255.0f;
    geometryClearValues[0].color.setFloat32({clearColor[0], clearColor[1], clearColor[2], clearColor[3]});
    geometryClearValues[1].color.setFloat32({0.0f, 0.0f, 0.0f, 0.0f});
    geometryClearValues[2].color.setFloat32({0.0f, 0.0f, 0.0f, 0.0f});
    geometryClearValues[3].color.setFloat32({0.0f, 0.0f, 0.0f, 1.0f});
    geometryClearValues[4].depthStencil.setDepth(1.0f);
    geometryClearValues[4].depthStencil.setStencil(0);

    vk::RenderPassBeginInfo geometryRenderPassBeginInfo  = vks::initializers::renderPassBeginInfo();
    geometryRenderPassBeginInfo.renderPass               = renderPass;
    geometryRenderPassBeginInfo.framebuffer              = gBuffer.framebuffer;
    geometryRenderPassBeginInfo.renderArea.extent.width  = surfaceWidth;
    geometryRenderPassBeginInfo.renderArea.extent.height = SurfaceHeight;
    geometryRenderPassBeginInfo.clearValueCount          = static_cast<uint32_t>(geometryClearValues.size());
    geometryRenderPassBeginInfo.pClearValues             = geometryClearValues.data();

    vk::CommandBuffer& cmd = geometryCmdBuffers[currentImage];

    // start recording
    cmd.begin(geometryCmdBufInfo);
    timings.resetFrame(cmd);
    timings.enterSection("TRANSFER", cmd);

    // add commands for transfer of sample data
    renderingInterface->transfer(cmd, cam);
    timings.leaveSection("TRANSFER", cmd);

    // add render commands
    cmd.beginRenderPass(&geometryRenderPassBeginInfo, vk::SubpassContents::eInline);

    vk::Viewport gBufferViewport = vks::initializers::viewport((float)surfaceWidth, (float)SurfaceHeight, 0.0f, 1.0f);
    cmd.setViewport(0, 1, &gBufferViewport);
    vk::Rect2D gBufferScissor = vks::initializers::rect2D(surfaceWidth, SurfaceHeight, 0, 0);
    cmd.setScissor(0, 1, &gBufferScissor);


    {
        // Actual rendering
        timings.enterSection("MAIN", cmd);
        renderingInterface->render(cmd, cam);
        timings.leaveSection("MAIN", cmd);
    }

    // end render commands
    cmd.endRenderPass();
    // end recording
    cmd.end();
}

//!
//! \brief VulkanDeferredRenderer::setupDrawCommandBuffer
//! \param currentImage indicates, which which cmdbuffer should be recorded
//! \param cam the camera to be used while rendering
//!
//! creates the command buffers for rendering from the gbuffer to the actual swapchain
//!
void VulkanDeferredRenderer::setupDrawCommandBuffer(int currentImage, Camera* cam)
{
    vk::CommandBufferBeginInfo cmdBufBeginInfo = vks::initializers::commandBufferBeginInfo();

    vk::ClearValue clearValues[2];

    clearValues[0].color.setFloat32({0.f, 0.f, 0.f, 1.f});
    clearValues[1].depthStencil.setDepth(1.0f);
    clearValues[1].depthStencil.setStencil(0);


    // renderpass begin info
    vk::RenderPassBeginInfo renderPassBeginInfo  = vks::initializers::renderPassBeginInfo();
    renderPassBeginInfo.renderPass               = lightingPass;
    renderPassBeginInfo.renderArea.offset.x      = 0;
    renderPassBeginInfo.renderArea.offset.y      = 0;
    renderPassBeginInfo.renderArea.extent.width  = surfaceWidth;
    renderPassBeginInfo.renderArea.extent.height = SurfaceHeight;
    renderPassBeginInfo.clearValueCount          = 2;
    renderPassBeginInfo.pClearValues             = clearValues;



    // set target framebuffer
    renderPassBeginInfo.framebuffer = frameBuffers[currentImage].framebuffer;


    // begin recording cmdBuffer
    drawCmdBuffers[currentImage].begin(cmdBufBeginInfo);

    lighting.updateUniformBuffers(drawCmdBuffers[currentImage], cam->proj, cam->view, lightDebug, debug);


    drawCmdBuffers[currentImage].beginRenderPass(&renderPassBeginInfo, vk::SubpassContents::eInline);

    // setup viewport & scissor
    vk::Viewport viewport = vks::initializers::viewport(surfaceWidth, SurfaceHeight, 0.0f, 1.0f);
    drawCmdBuffers[currentImage].setViewport(0, 1, &viewport);

    vk::Rect2D scissor = vks::initializers::rect2D(surfaceWidth, SurfaceHeight, 0, 0);
    drawCmdBuffers[currentImage].setScissor(0, 1, &scissor);

    lighting.setRenderLights(renderLights);
    lighting.renderLights(drawCmdBuffers[currentImage], cam);

    drawCmdBuffers[currentImage].endRenderPass();

    drawCmdBuffers[currentImage].end();
}

//! \brief VulkanDeferredRenderer::setupForwardCommandBuffer
//! \param currentImage indicates, which which cmdbuffer should be recorded
//! \param cam the camera to be used while rendering
//!
//! creates the command buffers for rendering parts of the scene in forward rendering e.g. transparent objects
//!
void VulkanDeferredRenderer::setupForwardCommandBuffer(int currentImage, Camera* cam)
{
    VulkanDeferredRenderingInterface* renderingInterface = dynamic_cast<VulkanDeferredRenderingInterface*>(rendering);
    SAIGA_ASSERT(renderingInterface);

    // record forwardRenderPass CmdBuffers
    VkCommandBufferBeginInfo fwdCmdBufInfo = vks::initializers::commandBufferBeginInfo();

    vec4 clearColor = vec4(57, 57, 57, 255) / 255.0f;

    vk::ClearValue clearValues[2];
    clearValues[0].color.setFloat32({clearColor[0], clearColor[1], clearColor[2], clearColor[3]});
    clearValues[1].depthStencil.setDepth(1.0f);
    clearValues[1].depthStencil.setStencil(0);

    vk::RenderPassBeginInfo fwdRenderPassBeginInfo  = vks::initializers::renderPassBeginInfo();
    fwdRenderPassBeginInfo.renderPass               = forwardPass;
    fwdRenderPassBeginInfo.renderArea.offset.x      = 0;
    fwdRenderPassBeginInfo.renderArea.offset.y      = 0;
    fwdRenderPassBeginInfo.renderArea.extent.width  = surfaceWidth;
    fwdRenderPassBeginInfo.renderArea.extent.height = SurfaceHeight;
    fwdRenderPassBeginInfo.clearValueCount          = 2;
    fwdRenderPassBeginInfo.pClearValues             = clearValues;


    vk::CommandBuffer& fwdCmd = forwardCmdBuffers[currentImage];

    // Set target frame buffer
    fwdRenderPassBeginInfo.framebuffer = frameBuffers[currentImage].framebuffer;

    fwdCmd.begin(fwdCmdBufInfo);
    timings.resetFrame(fwdCmd);
    timings.enterSection("TRANSFER", fwdCmd);

    renderingInterface->transferForward(fwdCmd, cam);

    timings.leaveSection("TRANSFER", fwdCmd);


    if (imGui && renderImgui) imGui->updateBuffers(fwdCmd, currentImage);

    fwdCmd.beginRenderPass(&fwdRenderPassBeginInfo, vk::SubpassContents::eInline);


    vk::Viewport fwdViewport = vks::initializers::viewport(surfaceWidth, SurfaceHeight, 0.0f, 1.0f);
    fwdCmd.setViewport(0, 1, &fwdViewport);

    vk::Rect2D fwdScissor = vks::initializers::rect2D(surfaceWidth, SurfaceHeight, 0, 0);
    fwdCmd.setScissor(0, 1, &fwdScissor);

    {
        // Actual rendering
        timings.enterSection("MAIN", fwdCmd);
        if (!debug) renderingInterface->renderForward(fwdCmd, cam);  // dont render forward if in gbuffer debug mode
        timings.leaveSection("MAIN", fwdCmd);
        // add imgui rendering here -- end of forward rendering
        timings.enterSection("IMGUI", fwdCmd);
        if (imGui && renderImgui) imGui->render(fwdCmd, currentImage);
        timings.leaveSection("IMGUI", fwdCmd);
    }

    fwdCmd.endRenderPass();
    fwdCmd.end();
    SAIGA_ASSERT(fwdCmd);
}

void VulkanDeferredRenderer::render(FrameSync& sync, int currentImage, Camera* cam)
{
    VulkanDeferredRenderingInterface* renderingInterface = dynamic_cast<VulkanDeferredRenderingInterface*>(rendering);
    SAIGA_ASSERT(renderingInterface);

    //    cout << "VulkanDeferredRenderer::render" << endl;
    if (imGui)
    {
        //        std::thread t([&](){
        imGui->beginFrame();
        ImGui::SetNextWindowSize(ImVec2(300, 400));
        ImGui::Begin("Deferred Renderer Settings");
        ImGui::Checkbox("Debug Mode", &debug);
        ImGui::Checkbox("Debug Lights", &lightDebug);
        ImGui::Checkbox("Render Lights", &renderLights);
        ImGui::End();


        renderingInterface->renderGUI();
        imGui->endFrame();
        //        });
        //        t.join();
    }
    if (!params.enableRTX)
    {
        // prepare the command buffers
        setupGeometryCommandBuffer(currentImage, cam);
        setupDrawCommandBuffer(currentImage, cam);
        setupForwardCommandBuffer(currentImage, cam);
        lighting.renderDepthMaps(shadowCmdBuffers[currentImage], renderingInterface);

        // TODO
        // think about synchronization ...

        // TODO dummy top of pipe
        vk::PipelineStageFlags gBufferSubmitPipelineStages =
            vk::PipelineStageFlagBits::eColorAttachmentOutput;  // TODO not right yet

        // offscreen rendering submitinfo and synchronization
        // wait for available image to start rendering TODO ??
        // signal that offscreen rendering is finished (geometrysemaphore)
        vk::SubmitInfo gBufferPassSubmitinfo       = vks::initializers::submitInfo();
        gBufferPassSubmitinfo.commandBufferCount   = 1;
        gBufferPassSubmitinfo.pCommandBuffers      = &geometryCmdBuffers[currentImage];
        gBufferPassSubmitinfo.pWaitDstStageMask    = &gBufferSubmitPipelineStages;
        gBufferPassSubmitinfo.pWaitSemaphores      = &sync.imageAvailable;
        gBufferPassSubmitinfo.waitSemaphoreCount   = 1;
        gBufferPassSubmitinfo.pSignalSemaphores    = &geometrySemaphore;
        gBufferPassSubmitinfo.signalSemaphoreCount = 1;

        // submit geometry pass
        base().mainQueue.submit(gBufferPassSubmitinfo, nullptr);



        vk::PipelineStageFlags shadowSubmitPipelineStages = vk::PipelineStageFlagBits::eColorAttachmentOutput;

        vk::SubmitInfo shadowSubmitInfo;
        //    submitInfo = vks::initializers::submitInfo();
        shadowSubmitInfo.pWaitDstStageMask    = &shadowSubmitPipelineStages;
        shadowSubmitInfo.waitSemaphoreCount   = 1;
        shadowSubmitInfo.pWaitSemaphores      = &geometrySemaphore;  // wait for finished geometry pass
        shadowSubmitInfo.signalSemaphoreCount = 1;
        shadowSubmitInfo.pSignalSemaphores    = &shadowSemaphore;

        shadowSubmitInfo.commandBufferCount = 1;
        shadowSubmitInfo.pCommandBuffers    = &shadowCmdBuffers[currentImage];  // use correct cmd buffer
        //    VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue, 1, &submitInfo, sync.frameFence));
        base().mainQueue.submit(shadowSubmitInfo, nullptr);



        vk::PipelineStageFlags submitPipelineStages =
            vk::PipelineStageFlagBits::eColorAttachmentOutput;  // TODO not right yet



        vk::SubmitInfo submitInfo;
        //    submitInfo = vks::initializers::submitInfo();
        submitInfo.pWaitDstStageMask    = &submitPipelineStages;
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.pWaitSemaphores      = &shadowSemaphore;  // wait for finished shadow pass
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = &deferredSemaphore;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &drawCmdBuffers[currentImage];  // use correct cmd buffer
        //    VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue, 1, &submitInfo, sync.frameFence));
        base().mainQueue.submit(submitInfo, nullptr);

        //    graphicsQueue.queue.submit(submitInfo,vk::Fence());


        // signal that rendering is complete etc
        std::array<vk::Semaphore, 2> signalSemaphores{sync.renderComplete, sync.defragMayStart};

        vk::SubmitInfo fwdSubmitInfo;
        //    submitInfo = vks::initializers::submitInfo();
        fwdSubmitInfo.pWaitDstStageMask    = &submitPipelineStages;
        fwdSubmitInfo.waitSemaphoreCount   = 1;
        fwdSubmitInfo.pWaitSemaphores      = &deferredSemaphore;  // wait for finished geometry pass
        fwdSubmitInfo.signalSemaphoreCount = 2;
        fwdSubmitInfo.pSignalSemaphores    = signalSemaphores.data();

        fwdSubmitInfo.commandBufferCount = 1;
        fwdSubmitInfo.pCommandBuffers    = &forwardCmdBuffers[currentImage];  // use correct cmd buffer
        //    VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue, 1, &submitInfo, sync.frameFence));
        base().mainQueue.submit(fwdSubmitInfo, sync.frameFence);

        timings.finishFrame(sync.defragMayStart);

        //    VK_CHECK_RESULT(swapChain.queuePresent(presentQueue, currentBuffer,  sync.renderComplete));
        base().finish_frame();
    }
    else
    {
        // prepare the command buffers
        setupGeometryCommandBuffer(currentImage, cam);
        setupDrawCommandBuffer(currentImage, cam);
        setupForwardCommandBuffer(currentImage, cam);
        lighting.renderDepthMaps(shadowCmdBuffers[currentImage], renderingInterface);

        // TODO
        // think about synchronization ...

        // TODO dummy top of pipe
        vk::PipelineStageFlags gBufferSubmitPipelineStages =
            vk::PipelineStageFlagBits::eColorAttachmentOutput;  // TODO not right yet

        // offscreen rendering submitinfo and synchronization
        // wait for available image to start rendering TODO ??
        // signal that offscreen rendering is finished (geometrysemaphore)
        vk::SubmitInfo gBufferPassSubmitinfo       = vks::initializers::submitInfo();
        gBufferPassSubmitinfo.commandBufferCount   = 1;
        gBufferPassSubmitinfo.pCommandBuffers      = &geometryCmdBuffers[currentImage];
        gBufferPassSubmitinfo.pWaitDstStageMask    = &gBufferSubmitPipelineStages;
        gBufferPassSubmitinfo.pWaitSemaphores      = &sync.imageAvailable;
        gBufferPassSubmitinfo.waitSemaphoreCount   = 1;
        gBufferPassSubmitinfo.pSignalSemaphores    = &geometrySemaphore;
        gBufferPassSubmitinfo.signalSemaphoreCount = 1;

        // submit geometry pass
        base().mainQueue.submit(gBufferPassSubmitinfo, nullptr);



        vk::PipelineStageFlags shadowSubmitPipelineStages = vk::PipelineStageFlagBits::eColorAttachmentOutput;

        vk::SubmitInfo shadowSubmitInfo;
        //    submitInfo = vks::initializers::submitInfo();
        shadowSubmitInfo.pWaitDstStageMask    = &shadowSubmitPipelineStages;
        shadowSubmitInfo.waitSemaphoreCount   = 1;
        shadowSubmitInfo.pWaitSemaphores      = &geometrySemaphore;  // wait for finished geometry pass
        shadowSubmitInfo.signalSemaphoreCount = 1;
        shadowSubmitInfo.pSignalSemaphores    = &shadowSemaphore;

        shadowSubmitInfo.commandBufferCount = 1;
        shadowSubmitInfo.pCommandBuffers    = &shadowCmdBuffers[currentImage];  // use correct cmd buffer
        //    VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue, 1, &submitInfo, sync.frameFence));
        base().mainQueue.submit(shadowSubmitInfo, nullptr);



        vk::PipelineStageFlags submitPipelineStages = vk::PipelineStageFlagBits::eBottomOfPipe;  // TODO not right yet



        vk::SubmitInfo submitInfo;
        //    submitInfo = vks::initializers::submitInfo();
        submitInfo.pWaitDstStageMask    = &submitPipelineStages;
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.pWaitSemaphores      = &shadowSemaphore;  // wait for finished shadow pass
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = &deferredSemaphore;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &drawCmdBuffers[currentImage];  // use correct cmd buffer
        //    VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue, 1, &submitInfo, sync.frameFence));
        base().mainQueue.submit(submitInfo, nullptr);


        // RTX

        // signal that rendering is complete etc
        std::array<vk::Semaphore, 2> signalSemaphores{sync.renderComplete, sync.defragMayStart};
        vk::PipelineStageFlags RTXsubmitPipelineStages = vk::PipelineStageFlagBits::eBottomOfPipe;

        vk::SubmitInfo RTXsubmitInfo;
        RTXsubmitInfo.pWaitDstStageMask    = &RTXsubmitPipelineStages;
        RTXsubmitInfo.waitSemaphoreCount   = 1;
        RTXsubmitInfo.pWaitSemaphores      = &deferredSemaphore;  // wait for finished geometry pass
        RTXsubmitInfo.signalSemaphoreCount = 1;
        RTXsubmitInfo.pSignalSemaphores    = &RTXSemaphore;


        raytracer.render(cam, lighting.firstSpotLight(), RTXCmdBuffers[currentImage],
                         swapChain.buffers[currentImage].image);

        //        VkCommandBufferBeginInfo cmdbegin2 = vks::initializers::commandBufferBeginInfo();
        //        drawCmdBuffers[currentImage].begin(cmdbegin2);
        //        drawCmdBuffers[currentImage].end();


        RTXsubmitInfo.pCommandBuffers    = &RTXCmdBuffers[currentImage];
        RTXsubmitInfo.commandBufferCount = 1;


        base().mainQueue.submit(RTXsubmitInfo, nullptr);


        vk::SubmitInfo fwdSubmitInfo;
        //    submitInfo = vks::initializers::submitInfo();
        fwdSubmitInfo.pWaitDstStageMask    = &submitPipelineStages;
        fwdSubmitInfo.waitSemaphoreCount   = 1;
        fwdSubmitInfo.pWaitSemaphores      = &RTXSemaphore;  // wait for finished geometry pass
        fwdSubmitInfo.signalSemaphoreCount = 2;
        fwdSubmitInfo.pSignalSemaphores    = signalSemaphores.data();

        fwdSubmitInfo.commandBufferCount = 1;
        fwdSubmitInfo.pCommandBuffers    = &forwardCmdBuffers[currentImage];  // use correct cmd buffer
        base().mainQueue.submit(fwdSubmitInfo, sync.frameFence);


        timings.finishFrame(sync.defragMayStart);

        base().finish_frame();
    }
}



}  // namespace Vulkan
}  // namespace Saiga
