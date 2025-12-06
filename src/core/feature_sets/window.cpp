#include <window.h>
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

Window& Window::operator=(Window&& other) noexcept {
    if (&other == this)
        return *this;
    
    pWindow = other.pWindow;
    vkSurface = other.vkSurface;
    vkInstance = other.vkInstance;
    extent = other.extent;
    other.pWindow = nullptr;
    other.vkSurface = VK_NULL_HANDLE;
    other.vkInstance = VK_NULL_HANDLE;

    return *this;
}

Window::Window(Window&& other) noexcept {
    *this = std::move(other);
}

Window::~Window() {
    if (vkInstance != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(vkInstance, vkSurface, nullptr);
        glfwDestroyWindow(pWindow);
    }
}