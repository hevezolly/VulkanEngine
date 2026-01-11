#pragma once

#include <common.h>
#include <feature_set.h>
#include <window.h>
#include <swap_chain.h>
#include <frame_buffer.h>
#include <messages.h>

struct PresentMsg{
    uint32_t swapChainIndex;
    Ref<Semaphore> wait;
};

struct API SwapChainSupport {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct API PresentFeature: FeatureSet, 
    CanHandle<EarlyInitMsg>,
    CanHandle<InitMsg>,
    CanHandle<DestroyMsg>,
    CanHandle<CollectInstanceRequirementsMsg>,
    CanHandle<CollectDeviceRequirementsMsg>,
    CanHandle<PresentMsg>,
    CanHandle<CollectRequiredQueueTypesMsg>,
    CanHandle<CheckDeviceAppropriateMsg>
{
    Window* window;
    SwapChain* swapChain;
    SwapChainSupport swapChainSupport;

    PresentFeature(RenderContext&, const WindowInitializer& windowArgs, const SwapChainInitializer& swapChaniArgs);

    virtual void OnMessage(EarlyInitMsg*);
    virtual void OnMessage(InitMsg*);
    virtual void OnMessage(DestroyMsg*);
    virtual void OnMessage(CollectInstanceRequirementsMsg*);
    virtual void OnMessage(CollectDeviceRequirementsMsg*);
    virtual void OnMessage(PresentMsg*);
    virtual void OnMessage(CheckDeviceAppropriateMsg*);
    virtual void OnMessage(CollectRequiredQueueTypesMsg*);

    uint32_t AcquireNextImage(Ref<Semaphore> imageReady);
    VkExtent2D swapChainExtent();

    uint32_t swapChainSize();

private: 
    WindowInitializer windowArgs;
    SwapChainInitializer swapChainArgs;

    void recreateSwapChain();
};