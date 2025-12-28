#include <stable_frame_rate.h>
#include <thread>

StableFPS::StableFPS(RenderContext& ctx, uint32_t fps): FeatureSet(ctx) {
    microsecondsPerFrame = std::chrono::microseconds(1000000 / fps);
}

void StableFPS::OnMessage(InitMsg*) {
    previousFrameBegin = std::chrono::high_resolution_clock::now();
}

void StableFPS::OnMessage(BeginFrameMsg*) {
    auto currentTime = std::chrono::high_resolution_clock::now();

    if (previousFrameBegin + microsecondsPerFrame > currentTime) {
        std::this_thread::sleep_for(previousFrameBegin + microsecondsPerFrame - currentTime);
    }

    previousFrameBegin = currentTime;
}