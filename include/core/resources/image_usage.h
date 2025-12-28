#pragma once
#include <common.h>

enum ImageUsage: VkFlags {
    ColorAttachment = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    TransferDst = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    Sampled = VK_IMAGE_USAGE_SAMPLED_BIT,
};

inline ImageUsage operator |(ImageUsage l, ImageUsage r) {
    return static_cast<ImageUsage>(static_cast<VkFlags>(l) | static_cast<VkFlags>(r));
}

inline ImageUsage operator &(ImageUsage l, ImageUsage r) {
    return static_cast<ImageUsage>(static_cast<VkFlags>(l) & static_cast<VkFlags>(r));
}