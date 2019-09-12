/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 *
 * Created by Mathias Harrer: mathias.mh.harrer@fau.de
 */

#pragma once

#include "saiga/core/camera/all.h"
#include "saiga/core/math/math.h"
#include "saiga/core/util/assert.h"
//#include "saiga/vulkan/CommandPool.h"
//#include "saiga/vulkan/FrameSync.h"
//#include "saiga/vulkan/Queue.h"
//#include "saiga/vulkan/Renderer.h"
//#include "saiga/vulkan/VulkanAsset.h"
//#include "saiga/vulkan/buffer/ColorBuffer.h"
//#include "saiga/vulkan/buffer/DepthBuffer.h"
//#include "saiga/vulkan/buffer/Framebuffer.h"
//#include "saiga/vulkan/renderModules/QuadRenderer.h"
//#include "saiga/vulkan/window/Window.h"

namespace Saiga
{
namespace Vulkan
{
namespace Lighting
{
class SAIGA_VULKAN_API PointLight
{
   public:
    PointLight(vec3 pos, vec3 dir, float angle);

    ~PointLight() {}

    void setPosition(vec3 pos);
    vec3 setDirection(vec3 dir);
    float setAngle(float angle);

    vec3 getPosition();
    vec3 getDirection();
    float getAngle();


    float angle;  // TODO changeto private
    vec3 light_dir;

   private:
    vec3 light_pos;
};
}  // namespace Lighting
}  // namespace Vulkan
}  // namespace Saiga
