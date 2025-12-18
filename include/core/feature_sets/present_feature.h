#pragma once

#include <common.h>
#include <feature_set.h>
#include <window.h>
#include <swap_chain.h>
#include <distinct_storage.h>
#include <frame_buffer.h>

struct API PresentFeature: FeatureSet {
    Window* window;
    SwapChain* swapChain;

    PresentFeature(RenderContext&, const WindowInitializer& windowArgs, const SwapChainInitializer& swapChaniArgs);

    virtual void PreInit();
    virtual void Init();
    virtual void Destroy();
    virtual void GetRequiredExtentions(std::vector<const char*>& buffer);

    uint32_t AcquireNextImage(Ref<Semaphore> imageReady);
    Ref<FrameBuffer> GetFrameBuffer(uint32_t swapChainImage, VkRenderPass renderPass);

    uint32_t swapChainSize();

    void Present(uint32_t swapChainImageIndex, Ref<Semaphore> wait);

private: 
    WindowInitializer windowArgs;
    SwapChainInitializer swapChainArgs;
    DistinctStorage<FrameBuffer, FrameBuffer_CtorArgs> swapChainFrameBuffers;

    void recreateSwapChain();
};