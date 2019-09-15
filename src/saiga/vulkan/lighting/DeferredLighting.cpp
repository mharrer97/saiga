/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 *
 * Created by Mathias Harrer: mathias.mh.harrer@fau.de
 */

#include "saiga/vulkan/lighting/DeferredLighting.h"

namespace Saiga
{
namespace Vulkan
{
namespace Lighting
{
DeferredLighting::DeferredLighting()
{
    // TODO create lgith meshes here
}
DeferredLighting::~DeferredLighting()
{
    attenuatedLightRenderer.destroy();
}
void DeferredLighting::init() {}

std::shared_ptr<PointLight> DeferredLighting::createPointLight()
{
    std::shared_ptr<PointLight> l = std::make_shared<PointLight>();
    pointLights.push_back(l);
    return l;
}

void DeferredLighting::removeLight(std::shared_ptr<PointLight> l)
{
    pointLights.erase(std::find(pointLights.begin(), pointLights.end(), l));
}


// TODO delete
std::shared_ptr<AttenuatedLight> DeferredLighting::createAttenuatedLight()
{
    std::shared_ptr<AttenuatedLight> l = std::make_shared<AttenuatedLight>();
    attenuatedLights.push_back(l);
    return l;
}

void DeferredLighting::removeLight(std::shared_ptr<AttenuatedLight> l)
{
    attenuatedLights.erase(std::find(attenuatedLights.begin(), attenuatedLights.end(), l));
}
}  // namespace Lighting

}  // namespace Vulkan

}  // namespace Saiga
