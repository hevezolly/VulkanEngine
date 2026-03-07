#pragma once
#include <feature_set.h>

struct FrameDispatcher: FeatureSet
{
    FrameDispatcher(RenderContext&, uint32_t framesInFlight=1);
    
    uint32_t getFramesInFlight();
    uint64_t frameIndex();
    uint32_t frameInFlightIndex();

    void BeginFrame();

private:
    uint32_t framesInFlight;
    uint64_t currentFrame;
};