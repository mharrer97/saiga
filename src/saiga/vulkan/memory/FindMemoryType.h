//
// Created by Peter Eichinger on 25.10.18.
//

#pragma once

#include <vulkan/vulkan.hpp>

inline uint32_t findMemoryType(vk::PhysicalDevice _pDev, uint32_t typeFilter, const vk::MemoryPropertyFlags& properties)
{
    // TODO: Move this to the appropriate classes and store the value
    vk::PhysicalDeviceMemoryProperties memProperties = _pDev.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

inline uint32_t getMemoryType(vk::PhysicalDevice _pDev, uint32_t typeBits, VkMemoryPropertyFlags properties,
                              VkBool32* memTypeFound = nullptr)
{
    VkPhysicalDeviceMemoryProperties memoryProperties = _pDev.getMemoryProperties();
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        if ((typeBits & 1) == 1)
        {
            if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                if (memTypeFound)
                {
                    *memTypeFound = true;
                }
                return i;
            }
        }
        typeBits >>= 1;
    }

    if (memTypeFound)
    {
        *memTypeFound = false;
        return 0;
    }
    else
    {
        throw std::runtime_error("Could not find a matching memory type");
    }
}
