/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#include "DepthBuffer.h"


namespace Saiga
{
namespace Vulkan
{
static const Memory::ImageType depth_buffer_type{vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                                 vk::MemoryPropertyFlagBits::eDeviceLocal};
void DepthBuffer::destroy()
{
    if (location)
    {
        base->memory.deallocateImage(depth_buffer_type, location);
        location = nullptr;
    }
}

void DepthBuffer::init(VulkanBase& base, int width, int height, bool sampleFromDepthBuffer)
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

        image_info.imageType             = vk::ImageType::e2D;
        image_info.format                = format;
        image_info.extent.width          = width;
        image_info.extent.height         = height;
        image_info.extent.depth          = 1;
        image_info.mipLevels             = 1;
        image_info.arrayLayers           = 1;
        image_info.samples               = vk::SampleCountFlagBits::e1;
        image_info.initialLayout         = vk::ImageLayout::eUndefined;
        image_info.usage                 = vk::ImageUsageFlagBits::eDepthStencilAttachment;
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
        viewInfo.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eDepth;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = 1;

        Memory::ImageData img_data(image_info, viewInfo, vk::ImageLayout::eUndefined);

        if (sampleFromDepthBuffer)
        {
            image_info.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled;

            vk::SamplerCreateInfo samplerCreateInfo = {};
            samplerCreateInfo.magFilter             = vk::Filter::eNearest;
            samplerCreateInfo.minFilter             = vk::Filter::eLinear;
            samplerCreateInfo.mipmapMode            = vk::SamplerMipmapMode::eLinear;
            samplerCreateInfo.addressModeU          = vk::SamplerAddressMode::eRepeat;
            samplerCreateInfo.addressModeV          = vk::SamplerAddressMode::eRepeat;
            samplerCreateInfo.addressModeW          = vk::SamplerAddressMode::eRepeat;
            samplerCreateInfo.mipLodBias            = 0.0f;
            samplerCreateInfo.compareOp             = vk::CompareOp::eNever;
            samplerCreateInfo.minLod                = 0.0f;
            // Max level-of-detail should match mip level count
            samplerCreateInfo.maxLod = 0.0f;
            // Only enable anisotropic filtering if enabled on the devicec
            samplerCreateInfo.maxAnisotropy    = 16;
            samplerCreateInfo.anisotropyEnable = VK_FALSE;
            samplerCreateInfo.borderColor      = vk::BorderColor::eIntOpaqueWhite;

            Memory::ImageData img_data_new(image_info, viewInfo, vk::ImageLayout::eUndefined, samplerCreateInfo);
            img_data = img_data_new;
        }
        // vk::MemoryRequirements mem_reqs;
        //
        //
        ///* Create image */
        ////        res = vkCreateImage(info.device, &image_info, NULL, &info.depth.image);
        // res = base.device.createImage(&image_info, nullptr, &depthimage);
        // SAIGA_ASSERT(res == vk::Result::eSuccess);
        ////        assert(res == VK_SUCCESS);
        //
        //
        ////        vkGetImageMemoryRequirements(info.device, info.depth.image, &mem_reqs);
        // mem_reqs = base.device.getImageMemoryRequirements(depthimage);
        //
        // location = base.memory.allocate(
        //    {vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal}, depthimage);
        //
        //
        //// allocateMemory(base, mem_reqs, vk::MemoryPropertyFlagBits::eDeviceLocal);
        //
        //
        // base.device.bindImageMemory(depthimage, location->memory, location->size);
        //
        //
        // viewInfo.image = depthimage;
        // res            = base.device.createImageView(&viewInfo, nullptr, &depthview);
        // SAIGA_ASSERT(res == vk::Result::eSuccess);

        location = base.memory.allocate(depth_buffer_type, img_data);
    }
}



}  // namespace Vulkan
}  // namespace Saiga
