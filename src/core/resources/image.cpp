#include "Image.h"
#include <common.h>
#include <iostream>

ImageView::ImageView(VkDevice device, Image* image):
device(device),
referencedImage(image)
{
    VkImageViewCreateInfo createInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    createInfo.image = image->vkImage;
    createInfo.viewType = image->description.depth == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_3D;
    createInfo.format = image->description.format;
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
    referencedImage = other.referencedImage;
    other.referencedImage = nullptr;
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
        referencedImage = nullptr;
        device = VK_NULL_HANDLE;
    }
}

Image::Image(
    VkImage readyImage, 
    VkDevice vkDevice,
    ImageDescription& description
):  vkImage(readyImage),
    description(description),
    memory(std::nullopt),
    device(VK_NULL_HANDLE)
{
    view = new ImageView(vkDevice, this);
}

Image::Image(
    VkImage img, 
    VkDevice vkDevice, 
    Memory&& memory, 
    ImageDescription& description
):  vkImage(img),
    description(description),
    memory(std::move(memory)),
    device(vkDevice)
{
    vkBindImageMemory(vkDevice, img, memory.vkMemory, memory.offset);
    view = new ImageView(vkDevice, this);
}

Image& Image::operator=(Image&& other) noexcept {
    if (&other == this)
        return *this;

    memory = std::move(other.memory);
    device = other.device;
    vkImage = other.vkImage;
    view = other.view;
    view->referencedImage = this;
    description = other.description;
    other.device = VK_NULL_HANDLE;
    other.memory = std::nullopt;
    other.vkImage = VK_NULL_HANDLE;
    other.view = nullptr;

    return *this;
}

Image::Image(Image&& other) noexcept {
    *this = std::move(other);
}

Image::~Image() {
    delete view;

    if (device != VK_NULL_HANDLE) {
        vkDestroyImage(device, vkImage, nullptr);
    }
}