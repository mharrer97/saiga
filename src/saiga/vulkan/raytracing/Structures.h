/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 *
 * Created by Mathias Harrer: mathias.mh.harrer@fau.de
 */

#pragma once

#include "saiga/export.h"

namespace Saiga
{
namespace Vulkan
{
namespace RTX
{
// Ray tracing acceleration structure
struct AccelerationStructure
{
    VkDeviceMemory memory                           = 0;
    VkAccelerationStructureNV accelerationStructure = 0;
    uint64_t handle                                 = 0;
};

// Ray tracing geometry instance
struct GeometryInstance
{
    Eigen::Matrix<float, 3, 4, Eigen::RowMajor> transform;
    uint32_t instanceId : 24;
    uint32_t mask : 8;
    uint32_t instanceOffset : 24;
    uint32_t flags : 8;
    uint64_t accelerationStructureHandle;
};

// Indices for the different ray tracing shader types used in this example
#define INDEX_RAYGEN 0
#define INDEX_MISS 1
#define INDEX_SHADOW_MISS 2
#define INDEX_CLOSEST_HIT 3
#define INDEX_SHADOW_HIT 4

#define NUM_SHADER_GROUPS 5

typedef enum Component
{
    VERTEX_COMPONENT_POSITION    = 0x0,
    VERTEX_COMPONENT_NORMAL      = 0x1,
    VERTEX_COMPONENT_COLOR       = 0x2,
    VERTEX_COMPONENT_UV          = 0x3,
    VERTEX_COMPONENT_TANGENT     = 0x4,
    VERTEX_COMPONENT_BITANGENT   = 0x5,
    VERTEX_COMPONENT_DATA_VEC3   = 0x6,
    VERTEX_COMPONENT_DUMMY_FLOAT = 0x7,
    VERTEX_COMPONENT_DUMMY_VEC4  = 0x8
} Component;

/** @brief Stores vertex layout components for model loading and Vulkan vertex input and atribute bindings  */
struct VertexLayout
{
   public:
    /** @brief Components used to generate vertices from */
    std::vector<Component> components;

    VertexLayout(std::vector<Component> components) { this->components = std::move(components); }

    uint32_t stride()
    {
        uint32_t res = 0;
        for (auto& component : components)
        {
            switch (component)
            {
                case VERTEX_COMPONENT_UV:
                    res += 2 * sizeof(float);
                    break;
                case VERTEX_COMPONENT_DUMMY_FLOAT:
                    res += sizeof(float);
                    break;
                case VERTEX_COMPONENT_DUMMY_VEC4:
                    res += 4 * sizeof(float);
                    break;
                default:
                    // All components except the ones listed above are made up of 3 floats
                    res += 3 * sizeof(float);
            }
        }
        return res;
    }
};

typedef enum RTXrenderMode
{
    REFLECTIONS,
    DIFFUSE
} RTXrenderMode;

}  // namespace RTX
}  // namespace Vulkan
}  // namespace Saiga
