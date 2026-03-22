#pragma once
#include <feature_set.h>

struct API FrameDispatcher: FeatureSet
{
    FrameDispatcher(RenderContext&, uint32_t framesInFlight=1);
    
    uint32_t getFramesInFlight();
    uint64_t frameIndex();
    uint32_t frameInFlightIndex();
    uint32_t nextFrameInFlightIndex();

    void BeginFrame();

private:
    uint32_t framesInFlight;
    uint64_t currentFrame;
};