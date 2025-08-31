#pragma once

#include "volk.h"
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include <common.h>
#include <iostream>
#include "device.h"

struct RenderContext {
    GLFWwindow* pWindow;
    Device* device;
    VkInstance vkInstance;
    VkSurfaceKHR vkSurface; 
    uint32_t windowWidth;
    uint32_t windowHeight;

    RenderContext(uint32_t windowWidth, uint32_t windowHeight, const char* windowTitle);
    ~RenderContext();
    
private:
#ifdef ENABLE_VULKAN_VALIDATION
    VkDebugUtilsMessengerEXT vkDebugMessenger;
#endif
};