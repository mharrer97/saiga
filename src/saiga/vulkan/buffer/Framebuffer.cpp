﻿/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#include "Framebuffer.h"


namespace Saiga
{
namespace Vulkan
{
void Framebuffer::destroy()
{
    if (framebuffer)
    {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
        framebuffer = nullptr;
    }
}

void Framebuffer::createColorDepthStencil(int width, int height, vk::ImageView color, vk::ImageView depthStencil,
                                          vk::RenderPass renderPass, vk::Device device)
{
    this->device = device;
    VkImageView attachments[2];

    // Depth/Stencil attachment is the same for all frame buffers
    //    attachments[1] = depthStencil.view;
    attachments[0] = color;
    attachments[1] = depthStencil;


    VkFramebufferCreateInfo frameBufferCreateInfo = {};
    frameBufferCreateInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.pNext                   = NULL;
    frameBufferCreateInfo.renderPass              = renderPass;
    frameBufferCreateInfo.attachmentCount         = 2;
    frameBufferCreateInfo.pAttachments            = attachments;
    frameBufferCreateInfo.width                   = width;
    frameBufferCreateInfo.height                  = height;
    frameBufferCreateInfo.layers                  = 1;


    VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &framebuffer));
}

void Framebuffer::createColor(int width, int height, vk::ImageView color, vk::RenderPass renderPass, vk::Device device)
{
    this->device = device;
    VkImageView attachments[1];

    // Depth/Stencil attachment is the same for all frame buffers
    //    attachments[1] = depthStencil.view;
    attachments[0] = color;


    VkFramebufferCreateInfo frameBufferCreateInfo = {};
    frameBufferCreateInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.pNext                   = NULL;
    frameBufferCreateInfo.renderPass              = renderPass;
    frameBufferCreateInfo.attachmentCount         = 1;
    frameBufferCreateInfo.pAttachments            = attachments;
    frameBufferCreateInfo.width                   = width;
    frameBufferCreateInfo.height                  = height;
    frameBufferCreateInfo.layers                  = 1;


    VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &framebuffer));
}

void Framebuffer::create(int width, int height, vk::RenderPass renderPass, vk::Device device)
{
    this->device = device;

    VkFramebufferCreateInfo frameBufferCreateInfo = {};
    frameBufferCreateInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.pNext                   = NULL;
    frameBufferCreateInfo.renderPass              = renderPass;
    frameBufferCreateInfo.attachmentCount         = 0;
    frameBufferCreateInfo.width                   = width;
    frameBufferCreateInfo.height                  = height;
    frameBufferCreateInfo.layers                  = 1;
    VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &framebuffer));
}

void Framebuffer::createGBuffer(int width, int height, vk::ImageView diffuse, vk::ImageView specular, vk::ImageView normal,
                   vk::ImageView additional, vk::ImageView depthStencil, vk::RenderPass renderPass, vk::Device device){
    this->device = device;
    VkImageView attachments[5];

    // Depth/Stencil attachment is the same for all frame buffers
    //    attachments[1] = depthStencil.view;
    attachments[0] = diffuse;
    attachments[1] = specular;
    attachments[2] = normal;
    attachments[3] = additional;
    attachments[4] = depthStencil;


    VkFramebufferCreateInfo frameBufferCreateInfo = {};
    frameBufferCreateInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.pNext                   = NULL;
    frameBufferCreateInfo.renderPass              = renderPass;
    frameBufferCreateInfo.attachmentCount         = 5;
    frameBufferCreateInfo.pAttachments            = attachments;
    frameBufferCreateInfo.width                   = width;
    frameBufferCreateInfo.height                  = height;
    frameBufferCreateInfo.layers                  = 1;


    VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &framebuffer));
}


}  // namespace Vulkan
}  // namespace Saiga
