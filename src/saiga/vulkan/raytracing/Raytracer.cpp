/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 *
 * Created by Mathias Harrer: mathias.mh.harrer@fau.de
 */

#include "Raytracer.h"

#include "saiga/vulkan/Shader/all.h"
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
    vkDestroyPipeline(base->device, pipeline, nullptr);
    vkDestroyPipelineLayout(base->device, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(base->device, descriptorSetLayout, nullptr);
    if (storageImageLocation)
    {
        base->memory.deallocateImage(color_buffer_type, storageImageLocation);
        storageImageLocation = nullptr;
    }
    vkFreeMemory(base->device, bottomLevelAS.memory, nullptr);
    vkFreeMemory(base->device, topLevelAS.memory, nullptr);
    vkDestroyAccelerationStructureNV(base->device, bottomLevelAS.accelerationStructure, nullptr);
    vkDestroyAccelerationStructureNV(base->device, topLevelAS.accelerationStructure, nullptr);
    vertexBuffer.destroy();
    indexBuffer.destroy();
    shaderBindingTable.destroy();
    ubo.destroy();
}

void Raytracer::init(VulkanBase& newBase, vk::Format SCColorFormat, uint32_t SCWidth, uint32_t SCHeight)
{
    this->base      = &newBase;
    SwapChainFormat = SCColorFormat;
    width           = SCWidth;
    height          = SCHeight;

    // Query the ray tracing properties of the current implementation, we will
    // need them later on
    rayTracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;
    VkPhysicalDeviceProperties2 deviceProps2{};
    deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceProps2.pNext = &rayTracingProperties;
    vkGetPhysicalDeviceProperties2(base->physicalDevice, &deviceProps2);

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

    createScene();
    createStorageImage();
    createUniformBuffer();
    createRayTracingPipeline();
    createShaderBindingTable();
    createDescriptorSets();
    prepared = true;
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

    VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
    memoryRequirementsInfo.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
    memoryRequirementsInfo.type                  = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
    memoryRequirementsInfo.accelerationStructure = bottomLevelAS.accelerationStructure;

    VkMemoryRequirements2 memoryRequirements2{};
    vkGetAccelerationStructureMemoryRequirementsNV(base->device, &memoryRequirementsInfo, &memoryRequirements2);

    VkMemoryAllocateInfo memoryAllocateInfo = vks::initializers::memoryAllocateInfo();
    memoryAllocateInfo.allocationSize       = memoryRequirements2.memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex      = findMemoryType(
        base->physicalDevice, memoryRequirements2.memoryRequirements.memoryTypeBits,
        vk::MemoryPropertyFlagBits::
            eDeviceLocal);  // vulkanDevice->getMemoryType(memoryRequirements2.memoryRequirements.memoryTypeBits,
                                 // VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(base->device, &memoryAllocateInfo, nullptr, &bottomLevelAS.memory));

    VkBindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo{};
    accelerationStructureMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
    accelerationStructureMemoryInfo.accelerationStructure = bottomLevelAS.accelerationStructure;
    accelerationStructureMemoryInfo.memory                = bottomLevelAS.memory;
    VK_CHECK_RESULT(vkBindAccelerationStructureMemoryNV(base->device, 1, &accelerationStructureMemoryInfo));

    VK_CHECK_RESULT(vkGetAccelerationStructureHandleNV(base->device, bottomLevelAS.accelerationStructure,
                                                       sizeof(uint64_t), &bottomLevelAS.handle));
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
    VkAccelerationStructureInfoNV accelerationStructureInfo{};
    accelerationStructureInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
    accelerationStructureInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
    accelerationStructureInfo.instanceCount = 1;
    accelerationStructureInfo.geometryCount = 0;

    VkAccelerationStructureCreateInfoNV accelerationStructureCI{};
    accelerationStructureCI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
    accelerationStructureCI.info  = accelerationStructureInfo;
    VK_CHECK_RESULT(vkCreateAccelerationStructureNV(base->device, &accelerationStructureCI, nullptr,
                                                    &topLevelAS.accelerationStructure));

    VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
    memoryRequirementsInfo.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
    memoryRequirementsInfo.type                  = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
    memoryRequirementsInfo.accelerationStructure = topLevelAS.accelerationStructure;

    VkMemoryRequirements2 memoryRequirements2{};
    vkGetAccelerationStructureMemoryRequirementsNV(base->device, &memoryRequirementsInfo, &memoryRequirements2);

    VkMemoryAllocateInfo memoryAllocateInfo = vks::initializers::memoryAllocateInfo();
    memoryAllocateInfo.allocationSize       = memoryRequirements2.memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex =
        findMemoryType(base->physicalDevice, memoryRequirements2.memoryRequirements.memoryTypeBits,
                       vk::MemoryPropertyFlagBits::eDeviceLocal);
    VK_CHECK_RESULT(vkAllocateMemory(base->device, &memoryAllocateInfo, nullptr, &topLevelAS.memory));

    VkBindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo{};
    accelerationStructureMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
    accelerationStructureMemoryInfo.accelerationStructure = topLevelAS.accelerationStructure;
    accelerationStructureMemoryInfo.memory                = topLevelAS.memory;
    VK_CHECK_RESULT(vkBindAccelerationStructureMemoryNV(base->device, 1, &accelerationStructureMemoryInfo));

    VK_CHECK_RESULT(vkGetAccelerationStructureHandleNV(base->device, topLevelAS.accelerationStructure, sizeof(uint64_t),
                                                       &topLevelAS.handle));
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
  */
}
VkResult Raytracer::createBuffer(VkBufferUsageFlags usageFlags, vk::MemoryPropertyFlags memoryPropertyFlags,
                                 vks::Buffer* buffer, VkDeviceSize size, void* data)
{
    buffer->device = base->device;

    // Create the buffer handle
    VkBufferCreateInfo bufferCreateInfo = vks::initializers::bufferCreateInfo(usageFlags, size);
    VK_CHECK_RESULT(vkCreateBuffer(base->device, &bufferCreateInfo, nullptr, &buffer->buffer));

    // Create the memory backing up the buffer handle
    VkMemoryRequirements memReqs;
    VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
    vkGetBufferMemoryRequirements(base->device, buffer->buffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    // Find a memory type index that fits the properties of the buffer
    memAlloc.memoryTypeIndex = findMemoryType(base->physicalDevice, memReqs.memoryTypeBits, memoryPropertyFlags);
    // getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
    VK_CHECK_RESULT(vkAllocateMemory(base->device, &memAlloc, nullptr, &buffer->memory));

    buffer->alignment           = memReqs.alignment;
    buffer->size                = memAlloc.allocationSize;
    buffer->usageFlags          = usageFlags;
    buffer->memoryPropertyFlags = static_cast<VkMemoryPropertyFlags>(memoryPropertyFlags);

    // If a pointer to the buffer data has been passed, map the buffer and copy over the data
    if (data != nullptr)
    {
        VK_CHECK_RESULT(buffer->map());
        memcpy(buffer->mapped, data, size);
        if ((static_cast<VkMemoryPropertyFlags>(memoryPropertyFlags) & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
            buffer->flush();

        buffer->unmap();
    }

    // Initialize a default descriptor that covers the whole buffer size
    buffer->setupDescriptor();

    // Attach the memory to the buffer object
    return buffer->bind();
}

void Raytracer::flushCommandBuffer(vk::CommandBuffer commandBuffer, bool free)
{
    /*if (commandBuffer == VK_NULL_HANDLE)
    {
        return;
    }*/

    // VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
    commandBuffer.end();

    vk::SubmitInfo submitInfo     = vks::initializers::submitInfo();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffer;

    // Create fence to ensure that the command buffer has finished executing
    vk::FenceCreateInfo fenceInfo = vks::initializers::fenceCreateInfo(VK_FLAGS_NONE);
    vk::Fence fence;

    // VK_CHECK_RESULT(vkCreateFence(logicalDevice, &fenceInfo, nullptr, &fence));
    base->device.createFence(&fenceInfo, nullptr, &fence);

    // Submit to the queue
    // VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
    base->mainQueue.submit(submitInfo, fence);
    // Wait for the fence to signal that command buffer has finished executing
    // VK_CHECK_RESULT(vkWaitForFences(logicalDevice, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));
    base->device.waitForFences(fence, VK_TRUE, std::numeric_limits<uint64_t>::max());

    // vkDestroyFence(logicalDevice, fence, nullptr);
    base->device.destroyFence(fence, nullptr);

    if (free)
    {
        base->mainQueue.commandPool.freeCommandBuffer(commandBuffer);
        // vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
    }
}


void Raytracer::createScene()
{
    // Setup vertices for a single triangle
    struct Vertex
    {
        float pos[3];
    };
    std::vector<Vertex> vertices = {{{1.0f, 1.0f, 0.0f}}, {{-1.0f, 1.0f, 0.0f}}, {{0.0f, -1.0f, 0.0f}}};

    // Setup indices
    std::vector<uint32_t> indices = {0, 1, 2};
    indexCount                    = static_cast<uint32_t>(indices.size());

    // Create buffers
    // For the sake of simplicity we won't stage the vertex data to the gpu memory
    // Vertex buffer
    //    VK_CHECK_RESULT(createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    //                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    //                                 &vertexBuffer, vertices.size() * sizeof(Vertex), vertices.data()));
    //    // Index buffer
    //    VK_CHECK_RESULT(createBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    //                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    //                                 &indexBuffer, indices.size() * sizeof(uint32_t), indices.data()));
    VK_CHECK_RESULT(createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                 &vertexBuffer, vertices.size() * sizeof(Vertex), vertices.data()));
    // Index buffer
    VK_CHECK_RESULT(createBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                 &indexBuffer, indices.size() * sizeof(uint32_t), indices.data()));

    /*
        Create the bottom level acceleration structure containing the actual scene geometry
    */
    VkGeometryNV geometry{};
    geometry.sType                              = VK_STRUCTURE_TYPE_GEOMETRY_NV;
    geometry.geometryType                       = VK_GEOMETRY_TYPE_TRIANGLES_NV;
    geometry.geometry.triangles.sType           = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
    geometry.geometry.triangles.vertexData      = vertexBuffer.buffer;
    geometry.geometry.triangles.vertexOffset    = 0;
    geometry.geometry.triangles.vertexCount     = static_cast<uint32_t>(vertices.size());
    geometry.geometry.triangles.vertexStride    = sizeof(Vertex);
    geometry.geometry.triangles.vertexFormat    = VK_FORMAT_R32G32B32_SFLOAT;
    geometry.geometry.triangles.indexData       = indexBuffer.buffer;
    geometry.geometry.triangles.indexOffset     = 0;
    geometry.geometry.triangles.indexCount      = indexCount;
    geometry.geometry.triangles.indexType       = VK_INDEX_TYPE_UINT32;
    geometry.geometry.triangles.transformData   = VK_NULL_HANDLE;
    geometry.geometry.triangles.transformOffset = 0;
    geometry.geometry.aabbs                     = {};
    geometry.geometry.aabbs.sType               = {VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV};
    geometry.flags                              = VK_GEOMETRY_OPAQUE_BIT_NV;

    createBLAS(&geometry);

    /*
        Create the top-level acceleration structure that contains geometry instance information
    */

    // Single instance with a 3x4 transform matrix for the ray traced triangle
    vks::Buffer instanceBuffer;

    Eigen::Matrix<float, 3, 4, Eigen::ColMajor> transform;
    transform << 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f;

    GeometryInstance geometryInstance{};
    geometryInstance.transform                   = transform;
    geometryInstance.instanceId                  = 0;
    geometryInstance.mask                        = 0xff;
    geometryInstance.instanceOffset              = 0;
    geometryInstance.flags                       = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
    geometryInstance.accelerationStructureHandle = bottomLevelAS.handle;

    VK_CHECK_RESULT(createBuffer(VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
                                 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                 &instanceBuffer, sizeof(GeometryInstance), &geometryInstance));

    createTLAS();

    /*
        Build the acceleration structure
    */

    // Acceleration structure build requires some scratch space to store temporary information
    VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
    memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
    memoryRequirementsInfo.type  = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;

    VkMemoryRequirements2 memReqBottomLevelAS;
    memoryRequirementsInfo.accelerationStructure = bottomLevelAS.accelerationStructure;
    vkGetAccelerationStructureMemoryRequirementsNV(base->device, &memoryRequirementsInfo, &memReqBottomLevelAS);

    VkMemoryRequirements2 memReqTopLevelAS;
    memoryRequirementsInfo.accelerationStructure = topLevelAS.accelerationStructure;
    vkGetAccelerationStructureMemoryRequirementsNV(base->device, &memoryRequirementsInfo, &memReqTopLevelAS);

    const VkDeviceSize scratchBufferSize =
        std::max(memReqBottomLevelAS.memoryRequirements.size, memReqTopLevelAS.memoryRequirements.size);

    vks::Buffer scratchBuffer;
    VK_CHECK_RESULT(createBuffer(VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
                                 vk::MemoryPropertyFlagBits::eDeviceLocal,  // VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                 &scratchBuffer, scratchBufferSize));

    // VkCommandBuffer cmdBuffer = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

    vk::CommandBuffer cmdBuffer = base->mainQueue.commandPool.createAndBeginOneTimeBuffer();
    /*
        Build bottom level acceleration structure
    */
    VkAccelerationStructureInfoNV buildInfo{};
    buildInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
    buildInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries   = &geometry;

    vkCmdBuildAccelerationStructureNV(cmdBuffer, &buildInfo, VK_NULL_HANDLE, 0, VK_FALSE,
                                      bottomLevelAS.accelerationStructure, VK_NULL_HANDLE, scratchBuffer.buffer, 0);


    VkMemoryBarrier memoryBarrier = vks::initializers::memoryBarrier();
    memoryBarrier.srcAccessMask =
        VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
    memoryBarrier.dstAccessMask =
        VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV,
                         VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0);

    /*
        Build top-level acceleration structure
    */
    buildInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
    buildInfo.pGeometries   = 0;
    buildInfo.geometryCount = 0;
    buildInfo.instanceCount = 1;

    vkCmdBuildAccelerationStructureNV(cmdBuffer, &buildInfo, instanceBuffer.buffer, 0, VK_FALSE,
                                      topLevelAS.accelerationStructure, VK_NULL_HANDLE, scratchBuffer.buffer, 0);

    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV,
                         VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0);

    // vulkanDevice->flushCommandBuffer(cmdBuffer, queue);
    flushCommandBuffer(cmdBuffer);



    scratchBuffer.destroy();
    instanceBuffer.destroy();
}

VkDeviceSize Raytracer::copyShaderIdentifier(uint8_t* data, const uint8_t* shaderHandleStorage, uint32_t groupIndex)
{
    const uint32_t shaderGroupHandleSize = rayTracingProperties.shaderGroupHandleSize;
    memcpy(data, shaderHandleStorage + groupIndex * shaderGroupHandleSize, shaderGroupHandleSize);
    data += shaderGroupHandleSize;
    return shaderGroupHandleSize;
}

void Raytracer::createShaderBindingTable()
{
    // Create buffer for the shader binding table
    const uint32_t sbtSize = rayTracingProperties.shaderGroupHandleSize * 3;
    // VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
    // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
    //                                           &shaderBindingTable, sbtSize));
    VK_CHECK_RESULT(createBuffer(VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, vk::MemoryPropertyFlagBits::eHostVisible,
                                 &shaderBindingTable, sbtSize));
    shaderBindingTable.map();

    auto shaderHandleStorage = new uint8_t[sbtSize];
    // Get shader identifiers
    VK_CHECK_RESULT(vkGetRayTracingShaderGroupHandlesNV(base->device, pipeline, 0, 3, sbtSize, shaderHandleStorage));
    auto* data = static_cast<uint8_t*>(shaderBindingTable.mapped);
    // Copy the shader identifiers to the shader binding table
    VkDeviceSize offset = 0;
    data += copyShaderIdentifier(data, shaderHandleStorage, INDEX_RAYGEN);
    data += copyShaderIdentifier(data, shaderHandleStorage, INDEX_MISS);
    data += copyShaderIdentifier(data, shaderHandleStorage, INDEX_CLOSEST_HIT);
    shaderBindingTable.unmap();
}


// TODO adjust to own shader handling
VkShaderModule Raytracer::loadShader(const char* fileName, VkDevice device)
{
    std::ifstream is(fileName, std::ios::binary | std::ios::in | std::ios::ate);

    if (is.is_open())
    {
        size_t size = is.tellg();
        is.seekg(0, std::ios::beg);
        char* shaderCode = new char[size];
        is.read(shaderCode, size);
        is.close();

        assert(size > 0);

        VkShaderModule shaderModule;
        VkShaderModuleCreateInfo moduleCreateInfo{};
        moduleCreateInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleCreateInfo.codeSize = size;
        moduleCreateInfo.pCode    = (uint32_t*)shaderCode;

        VK_CHECK_RESULT(vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule));

        delete[] shaderCode;

        return shaderModule;
    }
    else
    {
        std::cerr << "Error: Could not open shader file \"" << fileName << "\"" << std::endl;
        return VK_NULL_HANDLE;
    }
}

// TODO adjust to own shader handling
VkPipelineShaderStageCreateInfo Raytracer::loadShader(std::string fileName, VkShaderStageFlagBits stage)
{
    VkPipelineShaderStageCreateInfo shaderStage = {};
    shaderStage.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage.stage                           = stage;

    shaderStage.module = loadShader(fileName.c_str(), base->device);

    shaderStage.pName = "main";  // todo : make param
    assert(shaderStage.module != VK_NULL_HANDLE);
    shaderModules.push_back(shaderStage.module);
    return shaderStage;
}

void Raytracer::createDescriptorSets()
{
    std::vector<VkDescriptorPoolSize> poolSizes         = {{VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, 1},
                                                   {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
                                                   {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}};
    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 1);
    VK_CHECK_RESULT(vkCreateDescriptorPool(base->device, &descriptorPoolCreateInfo, nullptr, &descriptorPool));

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo =
        vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
    VK_CHECK_RESULT(vkAllocateDescriptorSets(base->device, &descriptorSetAllocateInfo, &descriptorSet));

    VkWriteDescriptorSetAccelerationStructureNV descriptorAccelerationStructureInfo{};
    descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
    descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
    descriptorAccelerationStructureInfo.pAccelerationStructures    = &topLevelAS.accelerationStructure;

    VkWriteDescriptorSet accelerationStructureWrite{};
    accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    // The specialized acceleration structure descriptor has to be chained
    accelerationStructureWrite.pNext           = &descriptorAccelerationStructureInfo;
    accelerationStructureWrite.dstSet          = descriptorSet;
    accelerationStructureWrite.dstBinding      = 0;
    accelerationStructureWrite.descriptorCount = 1;
    accelerationStructureWrite.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;

    VkDescriptorImageInfo storageImageDescriptor{};
    storageImageDescriptor.imageView   = storageImageLocation->data.view;
    storageImageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet resultImageWrite = vks::initializers::writeDescriptorSet(
        descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &storageImageDescriptor);
    VkWriteDescriptorSet uniformBufferWrite =
        vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &ubo.descriptor);

    std::vector<VkWriteDescriptorSet> writeDescriptorSets = {accelerationStructureWrite, resultImageWrite,
                                                             uniformBufferWrite};
    vkUpdateDescriptorSets(base->device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(),
                           0, VK_NULL_HANDLE);
}

void Raytracer::createRayTracingPipeline()
{
    VkDescriptorSetLayoutBinding accelerationStructureLayoutBinding{};
    accelerationStructureLayoutBinding.binding         = 0;
    accelerationStructureLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
    accelerationStructureLayoutBinding.descriptorCount = 1;
    accelerationStructureLayoutBinding.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_NV;

    VkDescriptorSetLayoutBinding resultImageLayoutBinding{};
    resultImageLayoutBinding.binding         = 1;
    resultImageLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    resultImageLayoutBinding.descriptorCount = 1;
    resultImageLayoutBinding.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_NV;

    VkDescriptorSetLayoutBinding uniformBufferBinding{};
    uniformBufferBinding.binding         = 2;
    uniformBufferBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformBufferBinding.descriptorCount = 1;
    uniformBufferBinding.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_NV;

    std::vector<VkDescriptorSetLayoutBinding> bindings(
        {accelerationStructureLayoutBinding, resultImageLayoutBinding, uniformBufferBinding});

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings    = bindings.data();
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(base->device, &layoutInfo, nullptr, &descriptorSetLayout));

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts    = &descriptorSetLayout;

    VK_CHECK_RESULT(vkCreatePipelineLayout(base->device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

    const uint32_t shaderIndexRaygen     = 0;
    const uint32_t shaderIndexMiss       = 1;
    const uint32_t shaderIndexClosestHit = 2;

    std::array<VkPipelineShaderStageCreateInfo, 3> shaderStages;
    shaderStages[shaderIndexRaygen] =
        loadShader(SAIGA_PROJECT_SOURCE_DIR "/shader/vulkan/raytracing/raygen.rgen.spv", VK_SHADER_STAGE_RAYGEN_BIT_NV);
    shaderStages[shaderIndexMiss] =
        loadShader(SAIGA_PROJECT_SOURCE_DIR "/shader/vulkan/raytracing/miss.rmiss.spv", VK_SHADER_STAGE_MISS_BIT_NV);
    shaderStages[shaderIndexClosestHit] = loadShader(
        SAIGA_PROJECT_SOURCE_DIR "/shader/vulkan/raytracing/closesthit.rchit.spv", VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);

    /*
            Setup ray tracing shader groups
    */
    std::array<VkRayTracingShaderGroupCreateInfoNV, 3> groups{};
    for (auto& group : groups)
    {
        // Init all groups with some default values
        group.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
        group.generalShader      = VK_SHADER_UNUSED_NV;
        group.closestHitShader   = VK_SHADER_UNUSED_NV;
        group.anyHitShader       = VK_SHADER_UNUSED_NV;
        group.intersectionShader = VK_SHADER_UNUSED_NV;
    }

    // Links shaders and types to ray tracing shader groups
    groups[INDEX_RAYGEN].type                  = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
    groups[INDEX_RAYGEN].generalShader         = shaderIndexRaygen;
    groups[INDEX_MISS].type                    = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
    groups[INDEX_MISS].generalShader           = shaderIndexMiss;
    groups[INDEX_CLOSEST_HIT].type             = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;
    groups[INDEX_CLOSEST_HIT].generalShader    = VK_SHADER_UNUSED_NV;
    groups[INDEX_CLOSEST_HIT].closestHitShader = shaderIndexClosestHit;

    VkRayTracingPipelineCreateInfoNV rayPipelineInfo{};
    rayPipelineInfo.sType             = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;
    rayPipelineInfo.stageCount        = static_cast<uint32_t>(shaderStages.size());
    rayPipelineInfo.pStages           = shaderStages.data();
    rayPipelineInfo.groupCount        = static_cast<uint32_t>(groups.size());
    rayPipelineInfo.pGroups           = groups.data();
    rayPipelineInfo.maxRecursionDepth = 1;
    rayPipelineInfo.layout            = pipelineLayout;
    VK_CHECK_RESULT(
        vkCreateRayTracingPipelinesNV(base->device, VK_NULL_HANDLE, 1, &rayPipelineInfo, nullptr, &pipeline));
}

void Raytracer::updateUniformBuffers(Camera* cam)
{
    uniformData.projInverse = inverse(cam->proj);
    uniformData.viewInverse = inverse(cam->view);
    memcpy(ubo.mapped, &uniformData, sizeof(uniformData));
}

void Raytracer::createUniformBuffer()
{
    VK_CHECK_RESULT(createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                 &ubo, sizeof(uniformData), &uniformData));
    VK_CHECK_RESULT(ubo.map());

    // updateUniformBuffers(cam);
}

void Raytracer::buildCommandBuffer(VkCommandBuffer cmd, VkImage targetImage)
{
    VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

    VkImageSubresourceRange subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};


    VK_CHECK_RESULT(vkBeginCommandBuffer(cmd, &cmdBufInfo));

    /*
            Dispatch the ray tracing commands
    */
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, pipelineLayout, 0, 1, &descriptorSet, 0, 0);

    // Calculate shader binding offsets, which is pretty straight forward in
    // our example
    VkDeviceSize bindingOffsetRayGenShader = rayTracingProperties.shaderGroupHandleSize * INDEX_RAYGEN;
    VkDeviceSize bindingOffsetMissShader   = rayTracingProperties.shaderGroupHandleSize * INDEX_MISS;
    VkDeviceSize bindingOffsetHitShader    = rayTracingProperties.shaderGroupHandleSize * INDEX_CLOSEST_HIT;
    VkDeviceSize bindingStride             = rayTracingProperties.shaderGroupHandleSize;

    vkCmdTraceRaysNV(cmd, shaderBindingTable.buffer, bindingOffsetRayGenShader, shaderBindingTable.buffer,
                     bindingOffsetMissShader, bindingStride, shaderBindingTable.buffer, bindingOffsetHitShader,
                     bindingStride, VK_NULL_HANDLE, 0, 0, width, height, 1);

    /*
            Copy raytracing output to swap chain image
    */

    // Prepare current swapchain image as transfer destination
    vks::tools::setImageLayout(cmd, targetImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               subresourceRange);

    // Prepare ray tracing output image as transfer source
    // vks::tools::setImageLayout(cmd, storageImageLocation->data.image, VK_IMAGE_LAYOUT_GENERAL,
    //                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange);
    storageImageLocation->data.transitionImageLayout(cmd, vk::ImageLayout::eTransferSrcOptimal);

    VkImageCopy copyRegion{};
    copyRegion.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copyRegion.srcOffset      = {0, 0, 0};
    copyRegion.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copyRegion.dstOffset      = {0, 0, 0};
    copyRegion.extent         = {width, height, 1};
    vkCmdCopyImage(cmd, storageImageLocation->data.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, targetImage,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    // Transition swap chain image back for presentation
    vks::tools::setImageLayout(cmd, targetImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                               subresourceRange);

    // Transition ray tracing output image back to general layout
    // vks::tools::setImageLayout(cmd, storageImageLocation->data.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    //                           VK_IMAGE_LAYOUT_GENERAL, subresourceRange);
    storageImageLocation->data.transitionImageLayout(cmd, vk::ImageLayout::eGeneral);

    //@todo: Default render pass setup willl overwrite contents
    // vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo,
    // VK_SUBPASS_CONTENTS_INLINE); drawUI(drawCmdBuffers[i]);
    // vkCmdEndRenderPass(drawCmdBuffers[i]);


    VK_CHECK_RESULT(vkEndCommandBuffer(cmd));
}

void Raytracer::render(Camera* cam, VkCommandBuffer cmd, VkImage targetImage)
{
    if (!prepared) return;
    updateUniformBuffers(cam);
    buildCommandBuffer(cmd, targetImage);
}

}  // namespace RTX
}  // namespace Vulkan
}  // namespace Saiga
