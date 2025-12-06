#pragma once

#include <volk.h>
#include <common.h>

struct API Image;

struct API ImageView {
    VkImageView vkImageView;
    Image* referencedImage;

    ImageView(VkDevice device, Image* image);

    RULE_5(ImageView)

private:
    VkDevice device;
};

struct API Image {
    VkImage vkImage;
    VkFormat format;
    uint32_t width;
    uint32_t height;
    uint32_t depth;

    ImageView* view;

    Image(VkImage readyImage, VkDevice device, VkFormat format, uint32_t width, uint32_t height, uint32_t depth = 1);
    // Image(VkDevice, VkFormat, uint32_t width, uint32_t height, uint32_t depth = 1);
    
    RULE_5(Image)

private:
    bool dispose = false;
    VkDevice device = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
};