#pragma once

#include "volk.h"
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

struct WindowInitializer {
    uint32_t width;
    uint32_t height;
    const char *hint;
};

struct Window {
    GLFWwindow* pWindow;
    VkSurfaceKHR vkSurface;
     
    uint32_t windowWidth;
    uint32_t windowHeight;

    Window(VkInstance vkInstance, const WindowInitializer& args);
    ~Window();
private:
    VkInstance vkInstance;
};