#pragma once

#include <common.h>
#include <resource_memory.h>
#include <optional>
#include <glm/glm.hpp>

void formatDepthStencilSupport(VkFormat format, bool& depth, bool& stencil);

struct API ImageDescription {
    VkFormat format;
    uint32_t width;
    uint32_t height;
    uint32_t depth=1;

    ImageDescription()=default;
    ImageDescription(VkFormat f, VkExtent2D extent, uint32_t d=1):
        format(f), width(extent.width), height(extent.height), depth(d){}
};

struct API Image;

struct API ImageView {
    VkImageView vkImageView;
    const Image* referencedImage;

    ImageView(VkDevice device, const Image* image, VkImageAspectFlags aspect);

    RULE_5(ImageView)

private:
    VkDevice device;
};

struct ResourceState {
    VkImageLayout currentLayout;
    VkAccessFlags currentAccess;
};

struct API Image {
    VkImage vkImage;
    ImageDescription description;
    ResourceState state;
    ImageView* view;
    VkClearValue clearValue;

    Image(VkImage readyImage, VkDevice device, const ImageDescription& description);
    Image(VkImage img, VkDevice device, Memory&& memory, const ImageDescription& description);
    
    RULE_5(Image)

private:
    VkDevice device = VK_NULL_HANDLE;
    std::optional<Memory> memory;
};