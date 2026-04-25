#pragma once

#include <common.h>

struct API WindowInitializer {
    uint32_t width;
    uint32_t height;
    const char *hint;
};

struct API Window {
    GLFWwindow* pWindow;
    VkSurfaceKHR vkSurface;
    
    VkExtent2D extent;

    Window(VkInstance vkInstance, const WindowInitializer& args);

    RULE_5(Window)
private:
    VkInstance vkInstance;
};