#pragma once

#include <volk.h>

struct Image;

struct ImageView {
    VkImageView vkImageView;

    ImageView(VkDevice device, Image* image);

    ImageView(const ImageView&) = delete;
    ImageView& operator=(const ImageView&) = delete;

    ImageView(ImageView&&) noexcept;
    ImageView& operator=(ImageView&&) noexcept;

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

    Image(VkImage readyImage, VkDevice device, VkFormat format, uint32_t width, uint32_t height, uint32_t depth = 1);
    // Image(VkDevice, VkFormat, uint32_t width, uint32_t height, uint32_t depth = 1);
    
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    Image(Image&&) noexcept;
    Image& operator=(Image&&) noexcept;

    ~Image();
private:
    bool dispose = false;
    VkDevice device = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
};