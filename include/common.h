#pragma once

#include <volk.h>
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>

#if defined(_WIN32) && defined(VULKAN_ENGINE_SHARED)
    #if defined(VulkanEngine_EXPORTS)
        #define API __declspec(dllexport)
    #else
        #define API __declspec(dllimport)
    #endif
#else
    #define API
#endif

enum ImageUsage: VkFlags {
    ColorAttachment = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    TransferDst = VK_IMAGE_USAGE_TRANSFER_DST_BIT
};

inline ImageUsage operator |(ImageUsage, ImageUsage);
inline ImageUsage operator &(ImageUsage, ImageUsage);

#define VK(call) {\
auto __result = (call);\
if (__result != VK_SUCCESS) {\
    std::stringstream ss;\
    ss << "Vk error in " << __FILE__ << " at line " << __LINE__ << " with error " << __result << std::endl;\
    throw std::runtime_error(ss.str());\
}\
}\
