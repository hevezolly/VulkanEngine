#pragma once

#include <common.h>
#include <feature_set.h>
#include <window.h>
#include <swap_chain.h>
#include <distinct_storage.h>
#include <frame_buffer.h>
#include <messages.h>

struct PresentMsg{
    uint32_t swapChainIndex;
    Ref<Semaphore> wait;
};

struct API PresentFeature: FeatureSet, 
    CanHandle<EarlyInitMsg>,
    CanHandle<InitMsg>,
    CanHandle<DestroyMsg>,
    CanHandle<CollectInstanceRequirementsMsg>,
    CanHandle<CollectDeviceRequirementsMsg>,
    CanHandle<PresentMsg>
{
    Window* window;
    SwapChain* swapChain;

    PresentFeature(RenderContext&, const WindowInitializer& windowArgs, const SwapChainInitializer& swapChaniArgs);

    virtual void OnMessage(EarlyInitMsg*);
    virtual void OnMessage(InitMsg*);
    virtual void OnMessage(DestroyMsg*);
    virtual void OnMessage(CollectInstanceRequirementsMsg*);
    virtual void OnMessage(CollectDeviceRequirementsMsg*);
    virtual void OnMessage(PresentMsg*);

    uint32_t AcquireNextImage(Ref<Semaphore> imageReady);
    Ref<FrameBuffer> GetFrameBuffer(uint32_t swapChainImage, VkRenderPass renderPass);

    uint32_t swapChainSize();

private: 
    WindowInitializer windowArgs;
    SwapChainInitializer swapChainArgs;
    DistinctStorage<FrameBuffer, FrameBuffer_CtorArgs> swapChainFrameBuffers;

    void recreateSwapChain();
};