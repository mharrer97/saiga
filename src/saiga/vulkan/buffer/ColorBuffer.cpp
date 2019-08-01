/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#include "ColorBuffer.h"


namespace Saiga
{
namespace Vulkan
{
static const Memory::ImageType color_buffer_type{vk::ImageUsageFlagBits::eColorAttachment,
                                                 vk::MemoryPropertyFlagBits::eDeviceLocal};
void ColorBuffer::destroy()
{
    if (location)
    {
        base->memory.deallocateImage(color_buffer_type, location);
        location = nullptr;
    }
}

void ColorBuffer::init(VulkanBase& base, int width, int height, vk::ImageUsageFlags usage)
{
    this->base = &base;
    {
        // color buffer
        vk::ImageCreateInfo image_info = {};
        vk::FormatProperties props;
        //        vkGetPhysicalDeviceFormatProperties(info.gpus[0], depth_format, &props);

        vk::PhysicalDevice physicalDevice = base.physicalDevice;
        props                             = physicalDevice.getFormatProperties(format);

        if (props.linearTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachment)
        {
            image_info.tiling = vk::ImageTiling::eLinear;
        }
        else if (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachment)
        {
            image_info.tiling = vk::ImageTiling::eOptimal;
        }
        else
        {
            /* Try other colorformats? */
            //std::cout << "VK_FORMAT_D16_UNORM Unsupported.\n";
            std::cout << "Unsupported format" << std::endl;
            exit(-1);
        }
        image_info.imageType             = vk::ImageType::e2D;
        image_info.format                = format;
        image_info.extent.width          = width;
        image_info.extent.height         = height;
        image_info.extent.depth          = 1;
        image_info.mipLevels             = 1;
        image_info.arrayLayers           = 1;
        image_info.samples               = vk::SampleCountFlagBits::e1;
        image_info.initialLayout         = vk::ImageLayout::eUndefined;
        image_info.usage                 = vk::ImageUsageFlagBits::eColorAttachment | usage;
        image_info.queueFamilyIndexCount = 0;
        image_info.pQueueFamilyIndices   = NULL;
        image_info.sharingMode           = vk::SharingMode::eExclusive;
        //        image_info.flags = 0;



        vk::ImageViewCreateInfo viewInfo = {};

        viewInfo.image                           = nullptr;
        viewInfo.viewType                        = vk::ImageViewType::e2D;
        viewInfo.format                          = format;
        viewInfo.components.r                    = vk::ComponentSwizzle::eR;
        viewInfo.components.g                    = vk::ComponentSwizzle::eG;
        viewInfo.components.b                    = vk::ComponentSwizzle::eB;
        viewInfo.components.a                    = vk::ComponentSwizzle::eA;
        viewInfo.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = 1;

        Memory::ImageData img_data(image_info, viewInfo, vk::ImageLayout::eUndefined);

        location = base.memory.allocate(color_buffer_type, img_data);
    }
}



}  // namespace Vulkan
}  // namespace Saiga
