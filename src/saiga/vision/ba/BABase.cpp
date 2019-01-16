﻿/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#include "BABase.h"

#include "saiga/imgui/imgui.h"

namespace Saiga
{
void BAOptions::imgui()
{
    ImGui::InputInt("maxIterations", &maxIterations);
    int currentItem             = (int)solverType;
    static const char* items[2] = {"Iterative", "Direct"};
    ImGui::Combo("SolverType", &currentItem, items, 2);
    solverType = (SolverType)currentItem;

    if (solverType == SolverType::Iterative)
    {
        ImGui::InputInt("maxIterativeIterations", &maxIterativeIterations);
        ImGui::InputDouble("iterativeTolerance", &iterativeTolerance);
    }

    ImGui::InputFloat("huberMono", &huberMono);
    ImGui::InputFloat("huberStereo", &huberStereo);
    ImGui::Checkbox("debugOutput", &debugOutput);
}



}  // namespace Saiga
