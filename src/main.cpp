#include "volk.h"
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

#include <iostream>

static void die(const char* msg) {
    std::cout << "Fatal: " << msg << std::endl;
    std::exit(1);
}

int main() {
    if (!glfwInit())
        die("failed to init glfw");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* win = glfwCreateWindow(800, 600, "VulkanEngine", nullptr, nullptr);
    
    if (!win)
        die("failed to create the window");

    if (volkInitialize() != VK_SUCCESS)
        die("Vulkan runtime not found (install/update your gpu drivers)");

    uint32_t extCount = 0;
    const char** exts = glfwGetRequiredInstanceExtensions(&extCount);
    if (!exts || extCount == 0)
        die("glfwGetRequiredInstanceExtensions failed");

    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "VulkanEngine";
    app.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    app.pEngineName = "VulkanEngine";
    app.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    app.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ci.pApplicationInfo = &app;
    ci.enabledExtensionCount = extCount;
    ci.ppEnabledExtensionNames = exts;

    VkInstance instance = VK_NULL_HANDLE;
    if (vkCreateInstance(&ci, nullptr, &instance) != VK_SUCCESS)
        die("vkCreateInstance failed");
    
    volkLoadInstance(instance);

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (glfwCreateWindowSurface(instance, win, nullptr, &surface) != VK_SUCCESS)
        die("glfwCreateWindowSurface failed");
    
    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(win);
    glfwTerminate();

    std::cout << "Success!" << std::endl;

    return 0;
}