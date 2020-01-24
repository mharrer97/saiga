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
#include "saiga/export.h"
#include "saiga/vulkan/VulkanAsset.h"
#include "saiga/vulkan/VulkanInitializers.hpp"
#include "saiga/vulkan/buffer/Buffer.h"
#include "saiga/vulkan/lighting/SpotLight.h"

#include "Structures.h"

#include <chrono>

namespace Saiga
{
namespace Vulkan
{
namespace RTX
{
class SAIGA_VULKAN_API RaytracerGB
{
   private:
    VulkanBase* base;
    vk::Format SwapChainFormat = vk::Format::eB8G8R8A8Unorm;
    uint32_t width             = 1280;
    uint32_t height            = 720;
    bool prepared              = false;
    bool hasGeometry           = false;
    RTXrenderMode renderMode   = REFLECTIONS;

   public:
    // TODO Forward Declarations ? ? ?
    PFN_vkCreateAccelerationStructureNV vkCreateAccelerationStructureNV;
    PFN_vkDestroyAccelerationStructureNV vkDestroyAccelerationStructureNV;
    PFN_vkBindAccelerationStructureMemoryNV vkBindAccelerationStructureMemoryNV;
    PFN_vkGetAccelerationStructureHandleNV vkGetAccelerationStructureHandleNV;
    PFN_vkGetAccelerationStructureMemoryRequirementsNV vkGetAccelerationStructureMemoryRequirementsNV;
    PFN_vkCmdBuildAccelerationStructureNV vkCmdBuildAccelerationStructureNV;
    PFN_vkCreateRayTracingPipelinesNV vkCreateRayTracingPipelinesNV;
    PFN_vkGetRayTracingShaderGroupHandlesNV vkGetRayTracingShaderGroupHandlesNV;
    PFN_vkCmdTraceRaysNV vkCmdTraceRaysNV;


    VkPhysicalDeviceRayTracingPropertiesNV rayTracingProperties{};

    // BLAS contains information about actual scene
    AccelerationStructure bottomLevelAS;
    // TLAS can contain multiple BLAS
    AccelerationStructure topLevelAS;

    // define contents of each component per vertex in the buffers
    VertexLayout vertexLayout = VertexLayout(
        {VERTEX_COMPONENT_POSITION, VERTEX_COMPONENT_NORMAL, VERTEX_COMPONENT_COLOR, VERTEX_COMPONENT_DATA_VEC3});
    // TODO outsource triangle handling
    // buffers to store the scene
    VulkanVertexColoredAsset* asset = nullptr;
    mat4 modelMatrix;
    vks::Buffer vertexBuffer;
    vks::Buffer indexBuffer;
    uint32_t vertexCount;
    uint32_t indexCount;
    // shader informations
    vks::Buffer shaderBindingTable;

    // TODO: change that up ? shader modules saved for deleting
    std::vector<VkShaderModule> shaderModues;

    //    struct StorageImage
    //    {
    //        VkDeviceMemory memory;
    //        VkImage image;
    //        VkImageView view;
    //        VkFormat format;
    //    } storageImage;
    Memory::ImageMemoryLocation* storageImageLocation = nullptr;

    // time point using chrono library
    // handed to shader to generate pseudorandom numbers
    std::chrono::time_point<std::chrono::system_clock> start, time;

    struct UniformData
    {
        mat4 viewInverse;
        mat4 projInverse;
        vec4 lightPos;
        vec4 attenuation;
        vec4 dir;
        vec4 specularCol;
        vec4 diffuseCol;
        float openingAngle;
        int time;
    } uniformData;
    vks::Buffer ubo;

    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet;
    VkDescriptorSetLayout descriptorSetLayout;

    RaytracerGB() {}
    ~RaytracerGB() { destroy(); }

    void destroy();

    //!
    //! \brief init initialize parameters. must be called before everything else
    //! \param base VulkanBase containinf devices etc
    //! \param SCColorFormat Color format of SwapChain Images
    //! \param SCWidth Width of SwapChain Images
    //! \param SCHeight Height of SwapChain Images
    //!
    void init(Saiga::Vulkan::VulkanBase& newBase, vk::Format SCColorFormat, uint32_t SCWidth, uint32_t SCHeight,
              RTXrenderMode renderMode, Saiga::Vulkan::Memory::ImageMemoryLocation* raster,
              Saiga::Vulkan::Memory::ImageMemoryLocation* normal, Saiga::Vulkan::Memory::ImageMemoryLocation* data,
              Saiga::Vulkan::Memory::ImageMemoryLocation* depth);

    //!
    //! \brief setGeometry initialize the geometry with the given asset
    //!
    void setGeometry(VulkanVertexColoredAsset* asset, mat4 model);

    //!
    //! \brief createStorageImage Set up a storage image that the ray generation shader will be writing to
    //!
    void createStorageImage();

    //!
    //! \brief createBLAS creates a Bottom Level Acceleration Structure from the gien geometry
    //! \param geometries geometries given to create BLAS
    //!
    void createBLAS(const VkGeometryNV* geometries);

    //!
    //! \brief createTLAS creates the Top Level Accelertion Structure
    //!
    void createTLAS();

    //!
    //! \brief createBuffer creates a buffer
    //! \param usageFlags
    //! \param memoryPropertyFlags
    //! \param buffer
    //! \param size
    //! \param data
    //! \return VkResult for Success checking
    //!
    VkResult createBuffer(VkBufferUsageFlags usageFlags, vk::MemoryPropertyFlags memoryPropertyFlags,
                          vks::Buffer* buffer, VkDeviceSize size, void* data = nullptr);

    //!
    //! \brief flushCommandBuffer used for ending and submitting a cmdbuffer instantly
    //! \param commandBuffer
    //! \param free
    //!
    void flushCommandBuffer(vk::CommandBuffer commandBuffer, bool free = true);

    //!
    //! \brief getVerticesFromAsset fills the vertices and inices vector with the information from the asset
    //! \param vertices
    //! \param indices
    //! \param indexCount
    //! \param vertexCount
    //!
    void getVerticesFromAsset(std::vector<float>& vertices, std::vector<uint32_t>& indices, uint32_t& vertexCount,
                              uint32_t& indexCount, Eigen::Matrix<float, 3, 4, Eigen::RowMajor>& transform);

    //!
    //! \brief createScene
    //! \param asset
    //!
    void createScene();

    VkDeviceSize copyShaderIdentifier(uint8_t* data, const uint8_t* shaderHandleStorage, uint32_t groupIndex);

    //!
    //! \brief createShaderBindingTable Create the Shader Binding Table that binds the programs and top-level
    //! acceleration structure
    //!
    void createShaderBindingTable();

    //!
    //! \brief createDescriptorSets Create the descriptor sets used for the ray tracing dispatch
    //!
    void createDescriptorSets(Saiga::Vulkan::Memory::ImageMemoryLocation* raster,
                              Saiga::Vulkan::Memory::ImageMemoryLocation* normal,
                              Saiga::Vulkan::Memory::ImageMemoryLocation* data,
                              Saiga::Vulkan::Memory::ImageMemoryLocation* depth);



    // List of shader modules created (stored for cleanup)
    std::vector<VkShaderModule> shaderModules;
    //! TODO adjust to own shader handling
    //! \brief loadShader
    //! \param fileName
    //! \param device
    //! \return
    //!
    VkShaderModule loadShader(const char* fileName, VkDevice device);

    //! TODO adjust to own shader handling
    //! \brief loadShader
    //! \param fileName
    //! \param stage
    //! \return
    //!
    VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage);



    //!
    //! \brief createRayTracingPipeline Create our raytracing pipeline
    //!
    void createRayTracingPipeline();

    //! TODO adjust to own ubo handling
    //! \brief updateUniformBuffers
    //!
    void updateUniformBuffers(Camera* cam, std::shared_ptr<Lighting::SpotLight> spotLight);

    //! TODO adjust to own ubo handling
    //! \brief createUniformBuffer Create the uniform buffer used to pass matrices to the ray tracing ray generation
    //! shader
    //!
    void createUniformBuffer();

    //!
    //! \brief buildCommandBuffers Command Buffer Generation
    //!
    void buildCommandBuffer(VkCommandBuffer cmd, VkImage targetImage);

    virtual void render(Camera* cam, std::shared_ptr<Lighting::SpotLight> spotLight, VkCommandBuffer cmd,
                        VkImage targetImage);
};
}  // namespace RTX
}  // namespace Vulkan
}  // namespace Saiga
