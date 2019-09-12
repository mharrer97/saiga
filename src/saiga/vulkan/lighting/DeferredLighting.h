/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 *
 * Created by Mathias Harrer: mathias.mh.harrer@fau.de
 */


#pragma once

#include "saiga/core/camera/camera.h"
#include "saiga/vulkan/VulkanAsset.h"
#include "saiga/vulkan/lighting/PointLight.h"


namespace Saiga
{
namespace Vulkan
{
namespace Lighting
{
struct DeferredLightingShaderNames
{
    std::string pointLightShader = "vulkan/lighting/lightPoint.glsl";
};

class SAIGA_VULKAN_API DeferredLighting
{
   private:
    VulkanVertexColoredAsset pointLightMesh;
    std::vector<std::shared_ptr<PointLight>> pointLights;

   public:
    int totalLights;
    int visibleLights;

    DeferredLighting();
    ~DeferredLighting();
    DeferredLighting& operator=(DeferredLighting& l) = delete;

    void init();  // give renderpass here?

    std::shared_ptr<PointLight> createPointLight();

    void removeLight(std::shared_ptr<PointLight> l);
};
}  // namespace Lighting
}  // namespace Vulkan
}  // namespace Saiga
