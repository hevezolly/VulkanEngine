#include <frame_dispatcher.h>
#include <render_context.h>

FrameDispatcher::FrameDispatcher(RenderContext& c, uint32_t framesInFlight):
    FeatureSet(c), framesInFlight(framesInFlight), currentFrame(0) 
{
    assert(framesInFlight > 0);
}

uint64_t FrameDispatcher::frameIndex() {
    return currentFrame;
}

uint32_t FrameDispatcher::frameInFlightIndex() {
    return currentFrame % framesInFlight;
}

void FrameDispatcher::BeginFrame() {
    currentFrame++;

    uint32_t frameInFlight = currentFrame % framesInFlight;

    BeginFrameMsg m {
        frameInFlight
    };

    BeginFrameLateMsg mLate {
        frameInFlight
    };

    context.Send(&m);
    context.Send(&mLate);
}   