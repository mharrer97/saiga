/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 *
 * Created by Mathias Harrer: mathias.mh.harrer@fau.de
 */

#include "saiga/core/framework/framework.h"
#include "saiga/core/util/easylogging++.h"
#include "saiga/vulkan/window/SDLWindow.h"

#include "sample.h"

#undef main

extern int maingsdgdfg();
/*int main(const int argc, const char* argv[])
{
    using namespace Saiga;

    {
        Saiga::WindowParameters windowParameters;
        Saiga::initSample(windowParameters.saigaParameters);
        windowParameters.fromConfigFile("config.ini");

        Saiga::Vulkan::SDLWindow window(windowParameters);
        //        Saiga::Vulkan::GLFWWindow window(windowParameters);

        Saiga::Vulkan::VulkanParameters vulkanParams;
        // vulkanParams.enableValidationLayer = true;
        vulkanParams.fromConfigFile("config.ini");

        Saiga::Vulkan::VulkanDeferredRenderer renderer(window, vulkanParams);



        VulkanExample example(window, renderer);

        MainLoopParameters params;
        params.fromConfigFile("config.ini");
        window.startMainLoop(params);

        renderer.waitIdle();
    }

    return 0;
}*/
