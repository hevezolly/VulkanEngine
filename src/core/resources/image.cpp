#include <image.h>
#include <common.h>
#include <iostream>

void formatDepthStencilSupport(VkFormat format, bool& depth, bool& stencil) {
    depth = format == VK_FORMAT_D16_UNORM ||
            format == VK_FORMAT_X8_D24_UNORM_PACK32 ||
            format == VK_FORMAT_D32_SFLOAT ||
            format == VK_FORMAT_D16_UNORM_S8_UINT ||
            format == VK_FORMAT_D24_UNORM_S8_UINT ||
            format == VK_FORMAT_D32_SFLOAT_S8_UINT;

    stencil = format == VK_FORMAT_S8_UINT ||
              format == VK_FORMAT_D16_UNORM_S8_UINT ||
              format == VK_FORMAT_D24_UNORM_S8_UINT ||
              format == VK_FORMAT_D32_SFLOAT_S8_UINT;
}

VkImageAspectFlags getAspect(VkFormat format) {
    bool supportsDepth;
    bool supportsStencil;
    formatDepthStencilSupport(format, supportsDepth, supportsStencil);

    if (!supportsDepth && !supportsStencil)
        return VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageAspectFlags aspect = 0;
    if (supportsDepth)
        aspect |= VK_IMAGE_ASPECT_DEPTH_BIT;
    if (supportsStencil)
        aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
    
    return aspect;
}

ImageView::ImageView(VkDevice device, const Image* image, VkImageAspectFlags aspect):
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

    createInfo.subresourceRange.aspectMask = aspect;
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
    const ImageDescription& description
):  vkImage(readyImage),
    description(description),
    memory(std::nullopt),
    device(VK_NULL_HANDLE),
    state{VK_IMAGE_LAYOUT_UNDEFINED, 0}
{
    auto aspect = getAspect(description.format);
    if ((aspect & VK_IMAGE_ASPECT_COLOR_BIT) == VK_IMAGE_ASPECT_COLOR_BIT)
        clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    else 
        clearValue.depthStencil = {1.0f, 0};
    view = new ImageView(vkDevice, this, aspect);
}

Image::Image(
    VkImage img, 
    VkDevice vkDevice, 
    Memory&& memory, 
    const ImageDescription& description
):  vkImage(img),
    description(description),
    memory(std::move(memory)),
    device(vkDevice),
    state{VK_IMAGE_LAYOUT_UNDEFINED, 0}
{
    vkBindImageMemory(vkDevice, img, this->memory.value().vkMemory, memory.offset);
    auto aspect = getAspect(description.format);
    if ((aspect & VK_IMAGE_ASPECT_COLOR_BIT) == VK_IMAGE_ASPECT_COLOR_BIT)
        clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    else 
        clearValue.depthStencil = {1.0f, 0};
    view = new ImageView(vkDevice, this, aspect);
}

Image& Image::operator=(Image&& other) noexcept {
    if (&other == this)
        return *this;

    memory = std::move(other.memory);
    device = other.device;
    vkImage = other.vkImage;
    view = other.view;
    state = other.state;
    clearValue = other.clearValue;
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