/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */


#pragma once

#include "saiga/vulkan/svulkan.h"

#include "Buffer.h"

namespace Saiga
{
namespace Vulkan
{
class SAIGA_VULKAN_API ColorBuffer
{
   public:
    ColorBuffer(vk::Format format = vk::Format::eR16G16B16A16Sfloat) : format(format) {}
    ~ColorBuffer() { destroy(); }

    // depth image
    vk::Format format;

    void init(Saiga::Vulkan::VulkanBase& base, int width, int height, vk::ImageUsageFlags usage,
              vk::Format format = vk::Format::eR16G16B16A16Sfloat);
    void destroy();

    Memory::ImageMemoryLocation* location = nullptr;

   private:
    VulkanBase* base;
};

}  // namespace Vulkan
}  // namespace Saiga
