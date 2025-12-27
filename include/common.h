#pragma once

#define GLFW_INCLUDE_NONE
#include <volk.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <cassert>

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
~name(); \
name(const name&) = delete; \
name& operator=(const name&) = delete; \
name(name&&) noexcept; \
name& operator=(name&&) noexcept;

#define RULE_NO(name) \
~name(); \
name(const name&) = delete; \
name& operator=(const name&) = delete; \
name(name&&) = delete; \
name& operator=(name&&) = delete;

enum struct API QueueType {
    Graphics,
    Transfer,
    Present,
    Compute,
    None
};

using QueueTypes=uint32_t;

static inline QueueTypes convert(QueueType t) {
    return (1 << (uint32_t)t);
}

static inline QueueTypes convert(QueueTypes t) {
    return t;
}

inline QueueTypes operator|(const QueueTypes lhs, const QueueType rhs) {
    return lhs | convert(rhs);
}

inline QueueTypes operator&(const QueueTypes lhs, const QueueType rhs) {
    return lhs & convert(rhs);
}

inline QueueTypes operator|(const QueueType lhs, const QueueTypes rhs) {
    return rhs | lhs;
}

inline QueueTypes operator&(const QueueType lhs, const QueueTypes rhs) {
    return rhs & lhs;
}

inline QueueTypes& operator |= (QueueTypes& lhs, const QueueType rhs) {
    lhs = lhs | rhs;
    return lhs;
}

inline QueueTypes operator | (const QueueType lhs, const QueueType rhs) {
    QueueTypes r = 0;
    r |= lhs;
    r |= rhs;
    return r;
}

#define VK(call) {\
auto __result = (call);\
if (__result != VK_SUCCESS) {\
    std::stringstream ss;\
    ss << "Vk error in " << __FILE__ << " at line " << __LINE__ << " with error " << __result << std::endl;\
    std::cout << ss.str() << std::endl;\
    throw std::runtime_error(ss.str());\
}\
}\

#ifdef ENGINE_LOG
#define LOG(str) std::cout << str << std::endl;
#define _S_ << " " <<
#else
#define LOG(str)
#endif
