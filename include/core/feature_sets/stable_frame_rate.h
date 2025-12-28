#pragma once

#include <common.h>
#include <feature_set.h>
#include <chrono>

struct API StableFPS: FeatureSet,
    CanHandle<BeginFrameMsg>,
    CanHandle<InitMsg>
{
    StableFPS(RenderContext&, uint32_t fps=60);
    void OnMessage(BeginFrameMsg*);
    void OnMessage(InitMsg*);

private:
    std::chrono::steady_clock::time_point previousFrameBegin;
    std::chrono::steady_clock::duration microsecondsPerFrame;
};