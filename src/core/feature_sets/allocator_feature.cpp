#include <allocator_feature.h>

Allocator::Allocator(RenderContext& ctx, uint32_t preallocatedSize):
    FeatureSet(ctx),
    allocatedSize(preallocatedSize),
    chunk(nullptr),
    freeOffset(0) 
{}

void Allocator::OnMessage(EarlyInitMsg*) {
    chunk = static_cast<char*>(malloc(allocatedSize));
}

void Allocator::OnMessage(DestroyMsg*) {
    free(chunk);
}

void Allocator::OnMessage(BeginFrameMsg* m) {
    if (m->inFlightFrame == 0)
        freeOffset = 0;
}