#pragma once

#include <common.h>
#include <resource_memory.h>
#include <optional>
#include <glm/glm.hpp>

struct API ImageDescription {
    VkFormat format;
    uint32_t width;
    uint32_t height;
    uint32_t depth=1;
};

struct API Image;

struct API ImageView {
    VkImageView vkImageView;
    Image* referencedImage;

    ImageView(VkDevice device, Image* image);

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

    Image(VkImage readyImage, VkDevice device, const ImageDescription& description);
    Image(VkImage img, VkDevice device, Memory&& memory, const ImageDescription& description);
    
    RULE_5(Image)

private:
    VkDevice device = VK_NULL_HANDLE;
    std::optional<Memory> memory;
};