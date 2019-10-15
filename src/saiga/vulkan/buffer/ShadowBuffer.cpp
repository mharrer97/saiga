/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#include "ShadowBuffer.h"


namespace Saiga
{
namespace Vulkan
{
static const Memory::ImageType depth_buffer_type{vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                                 vk::MemoryPropertyFlagBits::eDeviceLocal};
void ShadowBuffer::destroy()
{
    if (location)
    {
        base->memory.deallocateImage(depth_buffer_type, location);
        location = nullptr;
    }
}

void ShadowBuffer::init(VulkanBase& base, int width, int height)
{
    destroy();
    this->base = &base;
    {
        // depth buffer
        vk::ImageCreateInfo image_info = {};
        vk::FormatProperties props;
        //        vkGetPhysicalDeviceFormatProperties(info.gpus[0], depth_format, &props);

        vk::PhysicalDevice physicalDevice = base.physicalDevice;
        props                             = physicalDevice.getFormatProperties(format);

        if (props.linearTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
        {
            image_info.tiling = vk::ImageTiling::eLinear;
        }
        else if (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
        {
            image_info.tiling = vk::ImageTiling::eOptimal;
        }
        else
        {
            /* Try other depth formats? */
            std::cout << "VK_FORMAT_D16_UNORM Unsupported.\n";
            exit(-1);
        }
        image_info.imageType     = vk::ImageType::e2D;
        image_info.format        = format;
        image_info.extent.width  = width;
        image_info.extent.height = height;
        image_info.extent.depth  = 1;
        image_info.mipLevels     = 1;
        image_info.arrayLayers   = 1;
        image_info.samples       = vk::SampleCountFlagBits::e1;
        image_info.initialLayout = vk::ImageLayout::eUndefined;
        image_info.usage         = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled;
        image_info.queueFamilyIndexCount = 0;
        image_info.pQueueFamilyIndices   = NULL;
        image_info.sharingMode           = vk::SharingMode::eExclusive;
        image_info.tiling                = vk::ImageTiling::eOptimal;
        //        image_info.flags = 0;



        vk::ImageViewCreateInfo viewInfo = {};

        viewInfo.image                           = nullptr;
        viewInfo.viewType                        = vk::ImageViewType::e2D;
        viewInfo.format                          = format;
        viewInfo.components.r                    = vk::ComponentSwizzle::eR;
        viewInfo.components.g                    = vk::ComponentSwizzle::eG;
        viewInfo.components.b                    = vk::ComponentSwizzle::eB;
        viewInfo.components.a                    = vk::ComponentSwizzle::eA;
        viewInfo.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eDepth;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = 1;

        vk::SamplerCreateInfo samplerCreateInfo = {};
        samplerCreateInfo.magFilter             = vk::Filter::eLinear;
        samplerCreateInfo.minFilter             = vk::Filter::eLinear;
        samplerCreateInfo.mipmapMode            = vk::SamplerMipmapMode::eLinear;
        samplerCreateInfo.addressModeU          = vk::SamplerAddressMode::eClampToEdge;
        samplerCreateInfo.addressModeV          = vk::SamplerAddressMode::eClampToEdge;
        samplerCreateInfo.addressModeW          = vk::SamplerAddressMode::eClampToEdge;
        samplerCreateInfo.mipLodBias            = 0.0f;
        samplerCreateInfo.compareOp             = vk::CompareOp::eNever;
        samplerCreateInfo.minLod                = 0.0f;
        // Max level-of-detail should match mip level count
        samplerCreateInfo.maxLod = 1.0f;
        // Only enable anisotropic filtering if enabled on the devicec
        samplerCreateInfo.maxAnisotropy    = 1.0f;
        samplerCreateInfo.anisotropyEnable = VK_FALSE;
        samplerCreateInfo.borderColor      = vk::BorderColor::eFloatOpaqueWhite;

        Memory::ImageData img_data(image_info, viewInfo, vk::ImageLayout::eUndefined, samplerCreateInfo);


        location = base.memory.allocate(depth_buffer_type, img_data);
    }
}



}  // namespace Vulkan
}  // namespace Saiga
