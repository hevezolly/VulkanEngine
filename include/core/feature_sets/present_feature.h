#pragma once

#include <common.h>
#include <feature_set.h>
#include <window.h>
#include <swap_chain.h>

struct API PresentFeature: FeatureSet {
    Window* window;
    SwapChain* swapChain;

    PresentFeature(RenderContext&, const WindowInitializer& windowArgs, const SwapChainInitializer& swapChaniArgs);

    virtual void PreInit();
    virtual void Init();
    virtual void Destroy();
    virtual void GetRequiredExtentions(std::vector<const char*>& buffer);

private: 
    WindowInitializer windowArgs;
    SwapChainInitializer swapChainArgs;
};