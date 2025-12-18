#include <render_context.h>
#include <vector>
#include <debugging_feature.h>
#include <device.h>
#include <command_pool.h>
#include <synchronization.h>


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
    this->WithFeature<Device>()
        .WithFeature<CommandPool>()
        .WithFeature<Synchronization>();
}

void RenderContext::Initialize() {

    VK(volkInitialize());

    std::vector<const char*> requiredExtentions{};
    std::vector<const char*> requiredLayers{};

    for (int i = 0; i < _featureInitOrder.size(); i++) 
    {
        _features[_featureInitOrder[i]]->GetRequiredExtentions(requiredExtentions);
        _features[_featureInitOrder[i]]->GetRequiredLayers(requiredLayers);
    }

    VK(checkLayersSupport(requiredLayers))

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
    ci.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size());
    ci.ppEnabledLayerNames = requiredLayers.data();

    DebuggingFeature* debugFeature = TryGet<DebuggingFeature>();
    if (debugFeature)
        ci.pNext = &debugFeature->messengerCreateInfo;


    vkInstance = VK_NULL_HANDLE;
    VK(vkCreateInstance(&ci, nullptr, &vkInstance));

    volkLoadInstance(vkInstance);

    Send<EarlyInitMessage>(nullptr);
    Send<InitMessage>(nullptr);
}

RenderContext& RenderContext::operator=(RenderContext&& other) noexcept {

    if (&other == this)
        return *this;

    vkInstance = other.vkInstance;
    _initOrder = std::move(other._initOrder);
    _features = std::move(other._features);
    _featureInitOrder = std::move(other._featureInitOrder);
    _messageHandlers = std::move(other._messageHandlers);
    _messages = std::move(other._messages);
    other.vkInstance = VK_NULL_HANDLE;

    return *this;
}

RenderContext::RenderContext(RenderContext&& other) noexcept {
    *this = std::move(other);
}

VkDevice RenderContext::device() {
    return Get<Device>().device;
}

RenderContext::~RenderContext() {
    
    if (vkInstance != VK_NULL_HANDLE) {

        vkDeviceWaitIdle(device());

        for (int i = _initOrder.size() - 1; i >= 0; i--) {
            _initOrder[i].destroyer_func(_initOrder[i].object_ptr);
        }

        _initOrder.clear();

        for (int i = _featureInitOrder.size()-1; i >= 0; i--) 
        {
            _features[_featureInitOrder[i]]->Destroy();
        }
        
        _features.clear();
        _featureInitOrder.clear();

        vkDestroyInstance(vkInstance, nullptr);
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

        std::vector<MessageHandler>& handlers = _messageHandlers[m.type];
        for (int i = 0; i < handlers.size(); i++) {
            handlers[i].handler_func(handlers[i].handlerPtr, m.message);
        }
    }
    

    _handling = false;
}