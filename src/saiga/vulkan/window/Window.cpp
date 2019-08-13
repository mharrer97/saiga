﻿/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#include "Window.h"

#include "saiga/core/camera/camera.h"
#include "saiga/core/imgui/imgui.h"
#include "saiga/core/math/String.h"
#include "saiga/core/util/tostring.h"

#if defined(SAIGA_OPENGL_INCLUDED)
#    error OpenGL was included somewhere.
#endif


namespace Saiga
{
namespace Vulkan
{
VulkanWindow::VulkanWindow(WindowParameters windowParameters) : WindowBase(windowParameters) {}

VulkanWindow::~VulkanWindow() {}



void VulkanWindow::renderImGui(bool* p_open)
{
    if (!showImgui) return;

    p_open = &showImgui;

    int w = 340;
    int h = 240;
    ImGui::SetNextWindowPos(ImVec2(0, getHeight() - h), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_FirstUseEver);
    ImGui::Begin("Vulkan Window", &showImgui);

    mainLoop.renderImGuiInline();


    if (currentCamera)
    {
        ImGui::Text("Camera Position: %s", to_string(currentCamera->getPosition()).c_str());
        ImGui::Text("Camera Direction: %s", to_string(-make_vec3(currentCamera->getDirection())).c_str());
        if (ImGui::Button("Printf camera"))
        {
            //            std::cout << "camera.position = vec4(" << currentCamera->position << ";" << std::endl;

            auto t = currentCamera->position;
            auto q = currentCamera->rot;
            std::cout << "camera.position = vec4(" << t(0) << "," << t(1) << "," << t(2) << "," << 1 << ");" << std::endl;
            std::cout << "camera.rot = quat(" << q.w() << "," << q.x() << "," << q.y() << "," << q.z() << ");" << std::endl;
        }
    }



    ImGui::Checkbox("showRendererImgui", &showRendererImgui);

    if (showRendererImgui)
    {
        renderer->renderImGui(&showRendererImgui);
    }

    static bool showDemoWindow = false;

    ImGui::Checkbox("Show Demo Window", &showDemoWindow);

    if (showDemoWindow)
    {
        ImGui::ShowDemoWindow(&showDemoWindow);
    }

    ImGui::End();
}

void VulkanWindow::swap() {}


}  // namespace Vulkan
}  // namespace Saiga
