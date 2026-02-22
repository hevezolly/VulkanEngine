#include <frame_dispatcher.h>
#include <render_context.h>

FrameDispatcher::FrameDispatcher(RenderContext& c, uint32_t framesInFlight):
    FeatureSet(c), framesInFlight(framesInFlight), currentFrame(0) 
{
    assert(framesInFlight > 0);
}

uint64_t FrameDispatcher::frameIndex() {
    return currentFrame == 0 ? 0 : currentFrame - 1;
}

uint32_t FrameDispatcher::frameInFlightIndex() {
    return frameIndex() % framesInFlight;
}

void FrameDispatcher::BeginFrame() {
    currentFrame++;
    uint32_t currentFrameInFlight = frameInFlightIndex();

    BeginFrameMsg m {
        currentFrameInFlight
    };

    BeginFrameLateMsg mLate {
        currentFrameInFlight
    };

    context.Send(&m);
    context.Send(&mLate);
}   