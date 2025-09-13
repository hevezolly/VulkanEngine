#include "Image.h"
#include <common.h>
#include <iostream>

ImageView::ImageView(VkDevice device, Image* image):
device(device)
{
    VkImageViewCreateInfo createInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    createInfo.image = image->vkImage;
    createInfo.viewType = image->depth == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_3D;
    createInfo.format = image->format;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    VK(vkCreateImageView(device, &createInfo, nullptr, &vkImageView));
}

ImageView& ImageView::operator=(ImageView&& other) noexcept {

    if (&other == this)
        return *this;

    vkImageView = other.vkImageView;
    device = other.device;
    other.vkImageView = VK_NULL_HANDLE;
    other.device = VK_NULL_HANDLE;

    return *this;
}

ImageView::ImageView(ImageView&& other) noexcept {
    *this = std::move(other);
}

ImageView::~ImageView() {
    if (device != VK_NULL_HANDLE) {
        vkDestroyImageView(device, vkImageView, nullptr);
    }
}

Image::Image(VkImage readyImage, VkDevice device, VkFormat format, uint32_t width, uint32_t height, uint32_t depth):
vkImage(readyImage),
format(format),
width(width),
height(height),
depth(depth),
memory(VK_NULL_HANDLE),
device(device),
dispose(false)
{
    view = new ImageView(device, this);
}

Image& Image::operator=(Image&& other) noexcept {
    if (&other == this)
        return *this;

    dispose = other.dispose;
    device = other.device;
    memory = other.memory;
    vkImage = other.vkImage;
    view = other.view;
    format = other.format;
    width = other.width;
    height = other.height;
    depth = other.depth;

    other.dispose = false;
    other.device = VK_NULL_HANDLE;
    other.memory = VK_NULL_HANDLE;
    other.vkImage = VK_NULL_HANDLE;
    other.view = nullptr;

    return *this;
}

Image::Image(Image&& other) noexcept {
    *this = std::move(other);
}

Image::~Image() {
    delete view;

    if (!dispose) {
        return;
    }

    if (device != VK_NULL_HANDLE) {
        vkDestroyImage(device, vkImage, nullptr);
    }
    if (memory != VK_NULL_HANDLE) {
        vkFreeMemory(device, memory, nullptr);
    }
}