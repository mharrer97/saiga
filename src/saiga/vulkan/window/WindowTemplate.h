﻿/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#pragma once

#include "saiga/core/window/Interfaces.h"
#include "saiga/vulkan/VulkanDeferredRenderer.h"

#include "Window.h"

#ifdef SAIGA_USE_GLFW
#    include "GLFWWindow.h"
#else
namespace Saiga
{
using GLFWWindow = void;
}
#endif

#ifdef SAIGA_USE_SDL
#    include "SDLWindow.h"
#else
namespace Saiga
{
using SDLWindow = void;
}
#endif



namespace Saiga
{
namespace Vulkan
{
enum class WindowManagement
{
    SDL,
    GLFW
};

template <WindowManagement WM>
struct GetWindowType
{
};

template <>
struct GetWindowType<WindowManagement::SDL>
{
    using Type = SDLWindow;
};

template <>
struct GetWindowType<WindowManagement::GLFW>
{
    using Type = GLFWWindow;
};


template <WindowManagement WM, typename Renderer>
struct SAIGA_TEMPLATE StandaloneWindow : public Renderer::InterfaceType, public Updating
{
    using WindowManagment = typename GetWindowType<WM>::Type;

    StandaloneWindow(const std::string& config)
    {
        WindowParameters windowParameters;
        Saiga::Vulkan::VulkanParameters vulkanParams;
        typename Renderer::ParameterType rendererParameters;

        windowParameters.fromConfigFile(config);
        vulkanParams.fromConfigFile(config);
        rendererParameters.fromConfigFile(config);

        init(windowParameters, vulkanParams, rendererParameters);
    }

    StandaloneWindow(const WindowParameters& windowParameters, const VulkanParameters& vulkanParams,
                     const typename Renderer::ParameterType& rendererParameters)
    {
        init(windowParameters, vulkanParams, rendererParameters);
    }

    ~StandaloneWindow()
    {
        renderer.reset();
        window.reset();
    }

    void run(const MainLoopParameters& mainLoopParameters = MainLoopParameters())
    {
        // Everyhing is initilalized, we can run the main loop now!
        window->startMainLoop(mainLoopParameters);
        renderer->waitIdle();
    }

    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<WindowManagment> window;

   private:
    void init(const WindowParameters& windowParameters, const VulkanParameters& vulkanParams,
              const typename Renderer::ParameterType& rendererParameters)
    {
        window   = std::make_unique<WindowManagment>(windowParameters);
        renderer = std::make_unique<Renderer>(*window, vulkanParams);

        window->setUpdateObject(*this);
        renderer->setRenderObject(*this);
    }

    void init(const WindowParameters& windowParameters, VulkanParameters& vulkanParams,
              const VulkanDeferredRenderer::ParameterType& rendererParams)
    {
        window = std::make_unique<WindowManagment>(windowParameters);

        std::vector<std::string> additionalInstanceExtensions = {};

        if (rendererParams.enableRTX)
        {
            vulkanParams.deviceExtensions.push_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
            vulkanParams.deviceExtensions.push_back(VK_NV_RAY_TRACING_EXTENSION_NAME);
            additionalInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        }

        renderer = std::make_unique<Renderer>(*window, vulkanParams, additionalInstanceExtensions, rendererParams);

        window->setUpdateObject(*this);
        renderer->setRenderObject(*this);
    }
};

}  // namespace Vulkan
}  // namespace Saiga
