/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 *
 * Created by Mathias Harrer: mathias.mh.harrer@fau.de
 */

#include "Raytracer.h"

#include "saiga/vulkan/memory/FindMemoryType.h"
namespace Saiga
{
namespace Vulkan
{
namespace RTX
{
static const Memory::ImageType color_buffer_type{
    vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage, vk::MemoryPropertyFlagBits::eDeviceLocal};



void Raytracer::destroy()
{
    if (storageImageLocation)
    {
        base->memory.deallocateImage(color_buffer_type, storageImageLocation);
        storageImageLocation = nullptr;
    }
}

void Raytracer::init(VulkanBase& newBase, vk::Format SCColorFormat, uint32_t SCWidth, uint32_t SCHeight)
{
    this->base      = &newBase;
    SwapChainFormat = SCColorFormat;
    width           = SCWidth;
    height          = SCHeight;

    // Get VK_NV_ray_tracing related function pointers
    vkCreateAccelerationStructureNV = reinterpret_cast<PFN_vkCreateAccelerationStructureNV>(
        vkGetDeviceProcAddr(base->device, "vkCreateAccelerationStructureNV"));
    vkDestroyAccelerationStructureNV = reinterpret_cast<PFN_vkDestroyAccelerationStructureNV>(
        vkGetDeviceProcAddr(base->device, "vkDestroyAccelerationStructureNV"));
    vkBindAccelerationStructureMemoryNV = reinterpret_cast<PFN_vkBindAccelerationStructureMemoryNV>(
        vkGetDeviceProcAddr(base->device, "vkBindAccelerationStructureMemoryNV"));
    vkGetAccelerationStructureHandleNV = reinterpret_cast<PFN_vkGetAccelerationStructureHandleNV>(
        vkGetDeviceProcAddr(base->device, "vkGetAccelerationStructureHandleNV"));
    vkGetAccelerationStructureMemoryRequirementsNV =
        reinterpret_cast<PFN_vkGetAccelerationStructureMemoryRequirementsNV>(
            vkGetDeviceProcAddr(base->device, "vkGetAccelerationStructureMemoryRequirementsNV"));
    vkCmdBuildAccelerationStructureNV = reinterpret_cast<PFN_vkCmdBuildAccelerationStructureNV>(
        vkGetDeviceProcAddr(base->device, "vkCmdBuildAccelerationStructureNV"));
    vkCreateRayTracingPipelinesNV = reinterpret_cast<PFN_vkCreateRayTracingPipelinesNV>(
        vkGetDeviceProcAddr(base->device, "vkCreateRayTracingPipelinesNV"));
    vkGetRayTracingShaderGroupHandlesNV = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesNV>(
        vkGetDeviceProcAddr(base->device, "vkGetRayTracingShaderGroupHandlesNV"));
    vkCmdTraceRaysNV = reinterpret_cast<PFN_vkCmdTraceRaysNV>(vkGetDeviceProcAddr(base->device, "vkCmdTraceRaysNV"));

    createStorageImage();
}

void Raytracer::createStorageImage()
{
    vk::ImageCreateInfo image = vks::initializers::imageCreateInfo();
    image.imageType           = vk::ImageType::e2D;
    // TODO normally, VK_FORMAT_B8G8R8A8_UNORM is used as format. could go wrong -> add handling to adjust color format
    // to format of swapchain image.format            = swapChain.colorFormat;
    image.format        = SwapChainFormat;
    image.extent.width  = width;
    image.extent.height = height;
    image.extent.depth  = 1;
    image.mipLevels     = 1;
    image.arrayLayers   = 1;
    image.samples       = vk::SampleCountFlagBits::e1;
    image.tiling        = vk::ImageTiling::eOptimal;
    image.usage         = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage;
    image.initialLayout = vk::ImageLayout::eUndefined;


    // VK_CHECK_RESULT(vkCreateImage(device, &image, nullptr, &storageImage.image));

    //    VkMemoryRequirements memReqs;
    //    vkGetImageMemoryRequirements(device, storageImage.image, &memReqs);
    //    VkMemoryAllocateInfo memoryAllocateInfo = vks::initializers::memoryAllocateInfo();
    //    memoryAllocateInfo.allocationSize       = memReqs.size;
    //    memoryAllocateInfo.memoryTypeIndex =
    //        vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    //    VK_CHECK_RESULT(vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &storageImage.memory));
    //    VK_CHECK_RESULT(vkBindImageMemory(device, storageImage.image, storageImage.memory, 0));

    vk::ImageViewCreateInfo colorImageView = {};
    colorImageView.viewType                = vk::ImageViewType::e2D;
    colorImageView.format                  = SwapChainFormat;
    // colorImageView.subresourceRange                = {};
    colorImageView.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
    colorImageView.subresourceRange.baseMipLevel   = 0;
    colorImageView.subresourceRange.levelCount     = 1;
    colorImageView.subresourceRange.baseArrayLayer = 0;
    colorImageView.subresourceRange.layerCount     = 1;
    // TODO functioning with nullptr ???
    colorImageView.image = nullptr;  // storageImage.image;
    //    VK_CHECK_RESULT(vkCreateImageView(device, &colorImageView, nullptr, &storageImage.view));

    //    VkCommandBuffer cmdBuffer = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
    //    vks::tools::setImageLayout(cmdBuffer, storageImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
    //                               {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});
    //    vulkanDevice->flushCommandBuffer(cmdBuffer, queue);

    // TODO imagelayout ???
    Memory::ImageData img_data(image, colorImageView, vk::ImageLayout::eGeneral);

    storageImageLocation = base->memory.allocate(color_buffer_type, img_data);
}

void Raytracer::createBLAS(const VkGeometryNV* geometries)
{
    VkAccelerationStructureInfoNV accelerationStructureInfo{};
    accelerationStructureInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
    accelerationStructureInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
    accelerationStructureInfo.instanceCount = 0;
    accelerationStructureInfo.geometryCount = 1;
    accelerationStructureInfo.pGeometries   = geometries;

    VkAccelerationStructureCreateInfoNV accelerationStructureCI{};
    accelerationStructureCI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
    accelerationStructureCI.info  = accelerationStructureInfo;
    VK_CHECK_RESULT(vkCreateAccelerationStructureNV(base->device, &accelerationStructureCI, nullptr,
                                                    &bottomLevelAS.accelerationStructure));
    /*vk::AccelerationStructureInfoNV ASInfo{};
    ASInfo.type          = vk::AccelerationStructureTypeNV::eBottomLevel;
    ASInfo.instanceCount = 0;
    ASInfo.geometryCount = 1;
    ASInfo.pGeometries   = geometries;

    vk::AccelerationStructureCreateInfoNV ASCreateInfo{};
    ASCreateInfo.info = ASInfo;
    base->device.createAccelerationStructureNV(&ASCreateInfo, nullptr, &bottomLevelAS.accelerationStructure);
    SAIGA_ASSERT(bottomLevelAS.accelerationStructure);

        vk::AccelerationStructureMemoryRequirementsInfoNV memReqInfo{};
        memReqInfo.type                  = vk::AccelerationStructureMemoryRequirementsTypeNV::eObject;
        memReqInfo.accelerationStructure = bottomLevelAS.accelerationStructure;

        vk::MemoryRequirements2 memReq2{};
        base->device.getAccelerationStructureMemoryRequirementsNV(&memReqInfo, &memReq2);

        vk::MemoryAllocateInfo memAllocateInfo = vks::initializers::memoryAllocateInfo();
        memAllocateInfo.allocationSize         = memReq2.memoryRequirements.size;
        memAllocateInfo.memoryTypeIndex = findMemoryType(base->physicalDevice,
       memReq2.memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
        base->device.allocateMemory(&memAllocateInfo, nullptr, &bottomLevelAS.memory);
        SAIGA_ASSERT(bottomLevelAS.memory);

        vk::BindAccelerationStructureMemoryInfoNV accStructMemInfo{};
        accStructMemInfo.accelerationStructure = bottomLevelAS.accelerationStructure;
        accStructMemInfo.memory                = bottomLevelAS.memory;
        base->device.bindAccelerationStructureMemoryNV(1, &accStructMemInfo);

        base->device.getAccelerationStructureHandleNV(bottomLevelAS.accelerationStructure, sizeof(uint64_t),
                                                      &bottomLevelAS.handle);
        SAIGA_ASSERT(bottomLevelAS.handle);*/
}

void Raytracer::createTLAS()
{
  /*  vk::AccelerationStructureInfoNV ASInfo{};
    ASInfo.type          = vk::AccelerationStructureTypeNV::eTopLevel;
    ASInfo.instanceCount = 1;
    ASInfo.geometryCount = 0;

    vk::AccelerationStructureCreateInfoNV ASCreateInfo{};
    ASCreateInfo.info = ASInfo;
    base->device.createAccelerationStructureNV(&ASCreateInfo, nullptr, &topLevelAS.accelerationStructure);

    vk::AccelerationStructureMemoryRequirementsInfoNV memReqInfo{};
    memReqInfo.type                  = vk::AccelerationStructureMemoryRequirementsTypeNV::eObject;
    memReqInfo.accelerationStructure = topLevelAS.accelerationStructure;

    vk::MemoryRequirements2 memReq2{};
    base->device.getAccelerationStructureMemoryRequirementsNV(&memReqInfo, &memReq2);

    vk::MemoryAllocateInfo memAllocateInfo = vks::initializers::memoryAllocateInfo();
    memAllocateInfo.allocationSize         = memReq2.memoryRequirements.size;
    memAllocateInfo.memoryTypeIndex = findMemoryType(base->physicalDevice, memReq2.memoryRequirements.memoryTypeBits,
                                                     vk::MemoryPropertyFlagBits::eDeviceLocal);
    base->device.allocateMemory(&memAllocateInfo, nullptr, &topLevelAS.memory);
    SAIGA_ASSERT(topLevelAS.memory);

    vk::BindAccelerationStructureMemoryInfoNV ASMemInfo{};
    ASMemInfo.accelerationStructure = topLevelAS.accelerationStructure;
    ASMemInfo.memory                = topLevelAS.memory;
    base->device.bindAccelerationStructureMemoryNV(1, &ASMemInfo);

    base->device.getAccelerationStructureHandleNV(topLevelAS.accelerationStructure, sizeof(uint64_t),
                                                  &topLevelAS.handle);
    SAIGA_ASSERT(topLevelAS.handle);
*/}

  }  // namespace RTX
  }  // namespace Vulkan
  }  // namespace Saiga
