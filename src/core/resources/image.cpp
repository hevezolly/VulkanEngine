#include <image.h>
#include <common.h>
#include <iostream>
#include <render_context.h>
#include <resources.h>

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

ImageView::ImageView(RenderContext& context, const Image* image, const VkImageSubresourceRange& range):
context(&context),
referencedImage(image),
subresourceRange(range)
{
    VkImageViewCreateInfo createInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    createInfo.image = image->vkImage;
    createInfo.viewType = image->description.arrayLayers == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    createInfo.format = image->description.format;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    createInfo.subresourceRange = subresourceRange;

    VK(vkCreateImageView(context.device(), &createInfo, nullptr, &vkImageView));
}

ImageView& ImageView::operator=(ImageView&& other) noexcept {

    if (&other == this)
        return *this;

    vkImageView = other.vkImageView;
    context = other.context;
    referencedImage = other.referencedImage;
    subresourceRange = other.subresourceRange;
    other.referencedImage = nullptr;
    other.vkImageView = VK_NULL_HANDLE;
    other.context = nullptr;

    return *this;
}

ImageView::ImageView(ImageView&& other) noexcept {
    *this = std::move(other);
}

ImageView::~ImageView() {
    if (context != nullptr) {
        vkDestroyImageView(context->device(), vkImageView, nullptr);
        referencedImage = nullptr;
        context = nullptr;
    }
}

Image::Image(
    VkImage readyImage, 
    RenderContext& ctx,
    const ImageDescription& description
):  vkImage(readyImage),
    description(description),
    memory(std::nullopt),
    context(&ctx)
{
    auto aspect = getAspect(description.format);
    mainRange = VkImageSubresourceRange {
        aspect,
        0,
        description.mipLevels,
        0,
        description.arrayLayers
    };
    if ((aspect & VK_IMAGE_ASPECT_COLOR_BIT) == VK_IMAGE_ASPECT_COLOR_BIT)
        clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    else 
        clearValue.depthStencil = {1.0f, 0};
}

ImageView& Image::GetSubresource(const VkImageSubresourceRange& range) {
    auto it = subresources.find(range);
    if (it != subresources.end())
        return it->second;

    subresources.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(range),
        std::forward_as_tuple(*context, this, range));
    return subresources[range];
}

Image::Image(
    VkImage img, 
    RenderContext& ctx, 
    Memory&& memory, 
    const ImageDescription& description
):  vkImage(img),
    description(description),
    memory(std::move(memory)),
    context(&ctx)
{
    auto aspect = getAspect(description.format);
    mainRange = VkImageSubresourceRange {
        aspect,
        0,
        description.mipLevels,
        0,
        description.arrayLayers
    };
    vkBindImageMemory(ctx.device(), img, this->memory.value().vkMemory, memory.offset);
    if ((aspect & VK_IMAGE_ASPECT_COLOR_BIT) == VK_IMAGE_ASPECT_COLOR_BIT)
        clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    else 
        clearValue.depthStencil = {1.0f, 0};
}

Image& Image::operator=(Image&& other) noexcept {
    if (&other == this)
        return *this;

    memory = std::move(other.memory);
    context = other.context;
    vkImage = other.vkImage;
    mainRange = other.mainRange;
    subresources = std::move(other.subresources);
    for (auto& pair : subresources) {
        pair.second.referencedImage = this;
    }
    clearValue = other.clearValue;
    description = other.description;
    other.context = nullptr;
    other.memory = std::nullopt;
    other.vkImage = VK_NULL_HANDLE;

    return *this;
}

Image::Image(Image&& other) noexcept {
    *this = std::move(other);
}

Image::~Image() {
    subresources.clear();
    if (memory.has_value()) {
        vkDestroyImage(context->device(), vkImage, nullptr);
        context = nullptr;
    }
}

ImageView& Image::get(Mip mip, Layer layer) {

    VkImageSubresourceRange range = mainRange;
    range.baseArrayLayer = layer.layer;
    range.layerCount = layer.count;
    range.baseMipLevel = mip.level;
    range.levelCount = mip.count;

    ASSERT(range.baseArrayLayer >= mainRange.baseArrayLayer);
    ASSERT(range.baseMipLevel >= mainRange.baseMipLevel);
    ASSERT(range.baseArrayLayer + range.layerCount <= mainRange.baseArrayLayer + mainRange.layerCount);
    ASSERT(range.baseMipLevel + range.levelCount <= mainRange.baseMipLevel + mainRange.levelCount);

    return GetSubresource(range);
}