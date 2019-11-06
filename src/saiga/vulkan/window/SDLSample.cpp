﻿/*
 * Vulkan Example - imGui (https://github.com/ocornut/imgui)
 *
 * Copyright (C) 2017 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#include "saiga/config.h"
#ifdef SAIGA_USE_SDL
#    include "SDLSample.h"

#    if defined(SAIGA_OPENGL_INCLUDED)
#        error OpenGL was included somewhere.
#    endif

namespace Saiga
{
VulkanSDLExampleBase::VulkanSDLExampleBase() : StandaloneWindow("config.ini")
{
    float aspect = window->getAspectRatio();
    camera.setProj(60.0f, aspect, 0.1f, 50.0f, true);
    camera.setView(vec3(0, 1, 3), vec3(0, 0, 0), vec3(0, 1, 0));
    camera.rotationPoint = make_vec3(0);
    window->setCamera(&camera);
}

VulkanSDLExampleBase::~VulkanSDLExampleBase() {}

void VulkanSDLExampleBase::update(float dt)
{
    if (!ImGui::captureMouse()) camera.interpolate(dt, 0);
    if (!ImGui::captureKeyboard()) camera.update(dt);
}


void VulkanSDLExampleBase::renderGUI()
{
    window->renderImGui();
}


void VulkanSDLExampleBase::keyPressed(SDL_Keysym key)
{
    if (ImGui::captureKeyboard()) return;

    switch (key.scancode)
    {
        case SDL_SCANCODE_ESCAPE:
            window->close();
            break;
        case SDL_SCANCODE_G:
            renderer->setRenderImgui(!renderer->getRenderImgui());
            break;
        default:
            break;
    }
}

void VulkanSDLExampleBase::keyReleased(SDL_Keysym key) {}


// Deferred Example Base
VulkanDeferredSDLExampleBase::VulkanDeferredSDLExampleBase() : StandaloneWindow("config.ini")
{
    float aspect = window->getAspectRatio();
    camera.setProj(60.0f, aspect, 0.1f, 50.0f, true);
    camera.setView(vec3(0, 1, 3), vec3(0, 0, 0), vec3(0, 1, 0));
    camera.rotationPoint = make_vec3(0);
    window->setCamera(&camera);
}

VulkanDeferredSDLExampleBase::~VulkanDeferredSDLExampleBase() {}

void VulkanDeferredSDLExampleBase::update(float dt)
{
    if (!ImGui::captureMouse()) camera.interpolate(dt, 0);
    if (!ImGui::captureKeyboard()) camera.update(dt);
}


void VulkanDeferredSDLExampleBase::renderGUI()
{
    window->renderImGui();
}


void VulkanDeferredSDLExampleBase::keyPressed(SDL_Keysym key)
{
    if (ImGui::captureKeyboard()) return;

    switch (key.scancode)
    {
        case SDL_SCANCODE_ESCAPE:
            window->close();
            break;
        case SDL_SCANCODE_G:
            renderer->setRenderImgui(!renderer->getRenderImgui());
            break;
        default:
            break;
    }
}

void VulkanDeferredSDLExampleBase::keyReleased(SDL_Keysym key) {}
}  // namespace Saiga
#endif
