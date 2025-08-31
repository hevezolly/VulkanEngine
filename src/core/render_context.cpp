#include <render_context.h>
#include <vector>

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

static VkResult checkValidationLayersSupport() {
    uint32_t layerCount;
    VK(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));
    
    std::vector<VkLayerProperties> availableLayers(layerCount);
    VK(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()));

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties: availableLayers) {
            if (std::strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return VK_ERROR_LAYER_NOT_PRESENT;
        }
    }

    return VK_SUCCESS;
}

static std::vector<const char*> getRequiredExtentions() {
    uint32_t extCount = 0;
    const char** exts = glfwGetRequiredInstanceExtensions(&extCount);
    if (!exts || extCount == 0) 
    {
        std::cout << "glfwGetRequiredInstanceExtensions failed" << std::endl;
        std::exit(1);
    }

    std::vector<const char*> extentions(exts, exts + extCount);

#ifdef ENABLE_VULKAN_VALIDATION
    extentions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    return extentions;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    }

    return VK_FALSE;
}

RenderContext::RenderContext(uint32_t width, uint32_t height, const char* windowTitle) {
    if (!glfwInit()) {
        std::cout << "Failed to init glfw" << std::endl;
        std::exit(1);
    }

    windowWidth = width;
    windowHeight = height;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    pWindow = glfwCreateWindow(windowWidth, windowHeight, "VulkanEngine", nullptr, nullptr);

    if (!pWindow) {
        std::cout << "Failed to create window" << std::endl;
        std::exit(1);
    }

    VK(volkInitialize());

    std::vector<const char*> requiredExtentions = getRequiredExtentions();

    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "VulkanEngine";
    app.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    app.pEngineName = "VulkanEngine";
    app.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    app.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ci.pApplicationInfo = &app;
    ci.enabledExtensionCount = static_cast<uint32_t>(requiredExtentions.size());
    ci.ppEnabledExtensionNames = requiredExtentions.data();

#ifdef ENABLE_VULKAN_VALIDATION
    VK(checkValidationLayersSupport());
    ci.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    ci.ppEnabledLayerNames = validationLayers.data();

    VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    messengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    messengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    messengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    messengerCreateInfo.pfnUserCallback = debugCallback;

    ci.pNext = &messengerCreateInfo;
#endif

    vkInstance = VK_NULL_HANDLE;
    VK(vkCreateInstance(&ci, nullptr, &vkInstance));

    volkLoadInstance(vkInstance);

#ifdef ENABLE_VULKAN_VALIDATION
    VK(vkCreateDebugUtilsMessengerEXT(vkInstance, &messengerCreateInfo, nullptr, &vkDebugMessenger));
#endif

    vkSurface = VK_NULL_HANDLE;
    VK(glfwCreateWindowSurface(vkInstance, pWindow, nullptr, &vkSurface));

    device = new Device(vkInstance, vkSurface);
}

RenderContext::~RenderContext() {
    delete device;

    vkDestroySurfaceKHR(vkInstance, vkSurface, nullptr);

#ifdef ENABLE_VULKAN_VALIDATION
    vkDestroyDebugUtilsMessengerEXT(vkInstance, vkDebugMessenger, nullptr);
#endif
    vkDestroyInstance(vkInstance, nullptr);

    glfwDestroyWindow(pWindow);
    glfwTerminate();
}