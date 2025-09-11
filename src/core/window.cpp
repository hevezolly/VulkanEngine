#include "window.h"
#include <common.h>
#include <iostream>

Window::Window(VkInstance vkInstance, const WindowInitializer& initializer) {
    extent.width = initializer.width;
    extent.height = initializer.height;

    const char* hint = initializer.hint;
    if(!hint) {
        hint = "VulkanEngine";
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    pWindow = glfwCreateWindow(extent.width, extent.height, hint, nullptr, nullptr);

    if (!pWindow) {
        std::cout << "Failed to create window" << std::endl;
        std::exit(1);
    }

    this->vkInstance = vkInstance;

    vkSurface = VK_NULL_HANDLE;
    VK(glfwCreateWindowSurface(vkInstance, pWindow, nullptr, &vkSurface));
}

Window::~Window() {
    vkDestroySurfaceKHR(vkInstance, vkSurface, nullptr);
    glfwDestroyWindow(pWindow);
}