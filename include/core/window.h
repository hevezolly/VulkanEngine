#pragma once

#include "volk.h"
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include <common.h>

struct API WindowInitializer {
    uint32_t width;
    uint32_t height;
    const char *hint;
};

struct API  Window {
    GLFWwindow* pWindow;
    VkSurfaceKHR vkSurface;
    
    VkExtent2D extent;

    Window(VkInstance vkInstance, const WindowInitializer& args);
    Window(Window&&) noexcept;
    Window& operator=(Window&&) noexcept;

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    ~Window();
private:
    VkInstance vkInstance;
};