#include <render_context.h>
#include <vector>
#include <debugging_feature.h>
#include <device.h>
#include <command_pool.h>
#include <synchronization.h>
#include <resources.h>
#include <descriptor_pool.h>
#include <registry.h>


static VkResult checkLayersSupport(std::vector<const char*>& layers) {
    uint32_t layerCount;
    VK(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));
    
    std::vector<VkLayerProperties> availableLayers(layerCount);
    VK(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()));

    for (const char* layerName : layers) {
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

RenderContext::RenderContext(): _handling(false) {
#ifdef ENABLE_VULKAN_VALIDATION
    this->WithFeature<DebuggingFeature>();
#endif
    this->WithFeature<Allocator>()
        .WithFeature<Device>()
        .WithFeature<CommandPool>()
        .WithFeature<Synchronization>()
        .WithFeature<Resources>()
        .WithFeature<Descriptors>()
        .WithFeature<Registry>();
}

void RenderContext::Initialize() {

    VK(volkInitialize());

    std::vector<const char*> requiredExtensions{};
    std::vector<const char*> requiredLayers{};

    LOG("collecting instance data")

    Send(CollectInstanceRequirementsMsg{&requiredExtensions, &requiredLayers}, true);

    LOG("extensions: " << requiredExtensions.size())
    LOG("layers: " << requiredLayers.size())

    VK(checkLayersSupport(requiredLayers))

    apiVersion = VK_API_VERSION_1_2;

    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "VulkanEngine";
    app.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    app.pEngineName = "VulkanEngine";
    app.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    app.apiVersion = apiVersion;

    VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ci.pApplicationInfo = &app;
    ci.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    ci.ppEnabledExtensionNames = requiredExtensions.data();
    ci.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size());
    ci.ppEnabledLayerNames = requiredLayers.data();

    DebuggingFeature* debugFeature = TryGet<DebuggingFeature>();
    if (debugFeature)
        ci.pNext = &debugFeature->messengerCreateInfo;


    vkInstance = VK_NULL_HANDLE;
    VK(vkCreateInstance(&ci, nullptr, &vkInstance));

    volkLoadInstance(vkInstance);

    Send<EarlyInitMsg>(nullptr, true);
    Send<InitMsg>(nullptr, true);
}

VkDevice RenderContext::device() {
    return Get<Device>().device;
}

RenderContext::~RenderContext() {
    
    if (vkInstance != VK_NULL_HANDLE) {

        vkDeviceWaitIdle(device());

        Send<EarlyDestroyMsg>();

        for (int i = _initOrder.size() - 1; i >= 0; i--) {
            _initOrder[i].destroyer_func(_initOrder[i].object_ptr);
        }

        _initOrder.clear();

        Send<DestroyMsg>();
        
        _features.clear();
        _featureInitOrder.clear();
        _messageHandlers.clear();

        vkDestroyInstance(vkInstance, nullptr);
    }
}

void RenderContext::HandleMessageNow(Message&& msg) {
    std::vector<MessageHandler>& handlers = _messageHandlers[msg.type];
        
    for (int i = 0; i < handlers.size(); i++) {

        int index = msg.bottomToTop ? i : (handlers.size() - 1 - i);

        handlers[index].handler_func(handlers[index].handlerPtr, msg.message);
    }
}

void RenderContext::HandleMessages() {
    if (_handling)
        return;

    _handling = true;

    while (_messages.size() > 0)
    {
        Message m = _messages.front();
        _messages.pop();

        HandleMessageNow(std::move(m));
    }

    _handling = false;
}