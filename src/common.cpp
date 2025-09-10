#include "common.h"

inline ImageUsage operator |(ImageUsage l, ImageUsage r) {
    return static_cast<ImageUsage>(static_cast<VkFlags>(l) | static_cast<VkFlags>(r));
}

inline ImageUsage operator &(ImageUsage l, ImageUsage r) {
    return static_cast<ImageUsage>(static_cast<VkFlags>(l) | static_cast<VkFlags>(r));
}