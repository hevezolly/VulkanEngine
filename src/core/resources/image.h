#pragma once

#include <volk.h>

struct ImageView {
    VkImageView vkImageView;

    ImageView(VkDevice device, Image* image);
    ~ImageView();

private:
    VkDevice device;
};

struct Image {
    VkImage vkImage;
    VkFormat format;
    uint32_t width;
    uint32_t height;
    uint32_t depth;

    ImageView* view;

    Image(VkImage, VkDevice, VkFormat, uint32_t width, uint32_t height, uint32_t depth = 1);
    // Image(VkDevice, VkFormat, uint32_t width, uint32_t height, uint32_t depth = 1);
    ~Image();
private:
    bool autoCleanUp = false;
    VkDevice device = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
};