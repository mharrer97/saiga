﻿/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */


#include "Renderer.h"

#include "saiga/vulkan/window/Window.h"

namespace Saiga
{
namespace Vulkan
{
VulkanRenderer::VulkanRenderer(VulkanWindow& window, VulkanParameters vulkanParameters)
    : window(window), vulkanParameters(vulkanParameters)
{
    window.setRenderer(this);


    auto instanceExtensions = window.getRequiredInstanceExtensions();
    instance.create(instanceExtensions, vulkanParameters.enableValidationLayer);


    window.createSurface(instance, &surface);

    base().setPhysicalDevice(instance, instance.pickPhysicalDevice());

    vulkanParameters.physicalDeviceFeatures.fillModeNonSolid = VK_TRUE;
    vulkanParameters.physicalDeviceFeatures.wideLines        = VK_TRUE;
    base().createLogicalDevice(vulkanParameters, true);


    swapChain.connect(instance, base().physicalDevice, base().device);
    swapChain.initSurface(surface);

    state = State::INITIALIZED;
    base().initMemory(swapChain.imageCount);
    timings = FrameTimings(base().device, &base().mainQueue, &(base().memory));

    timings.registerFrameSection("TRANSFER", 0);
    timings.registerFrameSection("MAIN", 1);
    timings.registerFrameSection("IMGUI", 2);
}

VulkanRenderer::~VulkanRenderer()
{
    VLOG(3) << "Destroying VulkanRenderer";
    // Only wait until the queue is done.
    // All vulkan objects are destroyed in their destructor
    waitIdle();
}

void VulkanRenderer::init()
{
    std::cout << "Init VulkanRenderer -- START" << std::endl;
    SAIGA_ASSERT(state == State::INITIALIZED);
    //    createSwapChain();
    swapChain.create(&surfaceWidth, &SurfaceHeight, true);

    syncObjects.clear();
    syncObjects.resize(swapChain.imageCount);
    for (auto& sync : syncObjects)
    {
        sync.create(base().device);
    }

    currentBuffer  = 0;
    nextSyncObject = 0;

    if (vulkanParameters.enableImgui)
    {
        imGui.reset();
        imGui = window.createImGui(swapChainSize());
    }

    createBuffers(swapChainSize(), surfaceWidth, SurfaceHeight);

    timings.create(swapChainSize());
    // Everyting fine.
    // We can start rendering now :).
    state = State::RENDERABLE;
    std::cout << "Init VulkanRenderer -- FINISHED" << std::endl;
}

void VulkanRenderer::render(Camera* cam)
{
    if (state == State::RESET)
    {
        state = State::INITIALIZED;
    }


    if (state == State::INITIALIZED)
    {
        init();
    }

    timings.update();
    FrameSync& sync = syncObjects[nextSyncObject];
    sync.wait();
    timings.beginFrame(sync);

    VkResult err = swapChain.acquireNextImage(sync.imageAvailable, &currentBuffer);

    if (err == VK_ERROR_OUT_OF_DATE_KHR)
    {
        reset();
        return;
    }
    VK_CHECK_RESULT(err);


    render(sync, currentBuffer, cam);


    err = swapChain.queuePresent(base().mainQueue, currentBuffer, sync.renderComplete);
    if (err == VK_ERROR_OUT_OF_DATE_KHR)
    {
        reset();
        return;
    }
    VK_CHECK_RESULT(err);
    nextSyncObject = (nextSyncObject + 1) % syncObjects.size();
}

float VulkanRenderer::getTotalRenderTime()
{
    return window.mainLoop.renderCPUTimer.getTimeMS();
}


void VulkanRenderer::reset()
{
    SAIGA_ASSERT(state == State::RESET || state == State::RENDERABLE);
    waitIdle();
    state = State::RESET;
}

void VulkanRenderer::renderImGui(bool* p_open)
{
    ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_FirstUseEver);
    ImGui::Begin("Renderer Info", p_open, ImGuiWindowFlags_NoCollapse);

    base().renderGUI();
    base().memory.renderGUI();

    ImGui::End();
}


void VulkanRenderer::waitIdle()
{
    //    std::cout << "wait idle start" << std::endl;
    base().mainQueue.waitIdle();
    //    std::cout << "wait idle end" << std::endl;
    //    presentQueue.waitIdle();
    //    transferQueue.waitIdle();
}


}  // namespace Vulkan
}  // namespace Saiga
