#pragma once

#define GLFW_INCLUDE_NONE
#include <volk.h>
#include <GLFW/glfw3.h>
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

#define RULE_5(name) \
name(const name&) = delete;\
name& operator=(const name&) = delete;\
name(name&&) noexcept;\
name& operator=(name&&) noexcept;\
~name();

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
    std::cout << ss.str() << std::endl;\
    ss << "Vk error in " << __FILE__ << " at line " << __LINE__ << " with error " << __result << std::endl;\
    throw std::runtime_error(ss.str());\
}\
}\

#ifdef ENGINE_LOG
#define LOG(str) std::cout << str << std::endl;
#else
#define LOG(str)
#endif
