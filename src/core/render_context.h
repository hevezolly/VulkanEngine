#pragma once

#include "volk.h"
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include <common.h>
#include <iostream>
#include "device.h"
#include "window.h"
#include "swap_chain.h"

enum struct EngineFeatures : uint32_t {
    None = 0x0,
    WindowOutput = 0x1,
    GraphicsPipeline = 0x2,
};

inline EngineFeatures operator|(EngineFeatures l, EngineFeatures r) {
    return static_cast<EngineFeatures>(static_cast<uint32_t>(l) | static_cast<uint32_t>(r));
}

inline EngineFeatures operator&(EngineFeatures l, EngineFeatures r) {
    return static_cast<EngineFeatures>(static_cast<uint32_t>(l) & static_cast<uint32_t>(r));
}

struct RenderContextInitializer {
    EngineFeatures features;
    WindowInitializer windowDescription;
    SwapChainInitializer swapChainDescription;
};

struct RenderContext {
    VkInstance vkInstance;
    Window* window;
    Device* device;
    SwapChain* swapChain;

    RenderContext(const RenderContextInitializer& initializer);
    ~RenderContext();
    
private:
#ifdef ENABLE_VULKAN_VALIDATION
    VkDebugUtilsMessengerEXT vkDebugMessenger;
#endif
};