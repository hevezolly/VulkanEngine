#pragma once

#include <volk.h>
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include <common.h>
#include <iostream>
#include "device.h"
#include "window.h"
#include "swap_chain.h"
#include <map>
#include <handles.h>

enum struct API EngineFeatures : uint32_t {
    None = 0x0,
    WindowOutput = 0x1,
    GraphicsPipeline = 0x2,
};

inline API EngineFeatures operator|(EngineFeatures l, EngineFeatures r) {
    return static_cast<EngineFeatures>(static_cast<uint32_t>(l) | static_cast<uint32_t>(r));
}

inline API EngineFeatures operator&(EngineFeatures l, EngineFeatures r) {
    return static_cast<EngineFeatures>(static_cast<uint32_t>(l) & static_cast<uint32_t>(r));
}

struct API RenderContextInitializer {
    EngineFeatures features;
    WindowInitializer windowDescription;
    SwapChainInitializer swapChainDescription;
};

struct API RenderContext {
    VkInstance vkInstance;
    Window* window;
    Device* device;
    SwapChain* swapChain;

    RenderContext(const RenderContextInitializer& initializer);

    RULE_5(RenderContext)

    template<typename T>
    Ref<T> Register(T* item) {
        unsigned long id = _counter++;

        _items.insert(id, static_cast<void*>(item));
        _initOrder.push_back(id);

        return Ref<T> {id};
    }

    template<typename T>
    T* Get() {

    }
    
private:

    long _counter;
    std::map<unsigned long, void*> _items;
    std::vector<unsigned long> _initOrder;

#ifdef ENABLE_VULKAN_VALIDATION
    VkDebugUtilsMessengerEXT vkDebugMessenger;
#endif
};