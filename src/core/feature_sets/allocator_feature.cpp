#include <allocator_feature.h>

Allocator::Allocator(RenderContext& ctx, uint32_t preallocatedSize):
    FeatureSet(ctx),
    allocatedSize(preallocatedSize),
    chunk(nullptr),
    freeOffset(0) 
{}

void Allocator::OnMessage(InitMsg*) {
    chunk = static_cast<char*>(malloc(allocatedSize));
}

void Allocator::OnMessage(DestroyMsg*) {
    free(chunk);
}

void Allocator::OnMessage(BeginFrameMsg*) {
    freeOffset = 0;
}