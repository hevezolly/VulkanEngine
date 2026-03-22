#pragma once

#include <common.h>
#include <resource_memory.h>
#include <optional>
#include <glm/glm.hpp>
#include <resource_id.h>
#include <image_usage.h>

struct RenderContext;

void formatDepthStencilSupport(VkFormat format, bool& depth, bool& stencil);

struct API ImageDescription {
    VkFormat format;
    uint32_t width;
    uint32_t height;
    uint32_t depth=1;
    ImageUsage usage;

    ImageDescription()=default;
    ImageDescription(VkFormat f, ImageUsage u, VkExtent2D extent, uint32_t d=1):
        usage(u), format(f), width(extent.width), height(extent.height), depth(d){}
};

struct API Image;

struct API ImageView {
    VkImageView vkImageView;
    const Image* referencedImage;
    VkImageSubresourceRange subresourceRange;

    ImageView(RenderContext& context, const Image* image, VkImageAspectFlags aspect);

    RULE_5(ImageView)

private:
    RenderContext* context;
};

struct API Image {
    VkImage vkImage;
    ImageDescription description;
    ImageView* view;
    VkClearValue clearValue;

    Image(VkImage readyImage, RenderContext& context, const ImageDescription& description);
    Image(VkImage img, RenderContext& context, Memory&& memory, const ImageDescription& description);
    
    RULE_5(Image)

private:
    RenderContext* context = nullptr;
    std::optional<Memory> memory;
};