#pragma once
#include <feature_set.h>
#include <framed_storage.h>
#include <resources.h>
#include <buffer_region.h>

struct DynamicUniforms: FeatureSet,
    CanHandle<BeginFrameMsg>,
    CanHandle<InitMsg>
{
    DynamicUniforms(RenderContext& c, uint64_t preallocatedSize = 256):
        FeatureSet(c), capacityPerFrame(preallocatedSize)
    {} 

    void OnMessage(BeginFrameMsg*);
    void OnMessage(InitMsg*);
    
    BufferRegion AllocateRange(uint32_t size);
    
    template<typename T>
    BufferRegion Allocate(const T& value) {
        BufferRegion region = AllocateRange(sizeof(T));

        std::memcpy(
            region.buffer->memory.PersistentMap().data() + region.offset
            &value, sizeof(T)
        );

        return region;
    }

private:

    void Reallocate();

    uint64_t capacityPerFrame;
    uint32_t framesInFlight;
    VkDeviceSize minAlignment;
    VkDeviceSize maxSize;
    FramedStorage<BufferRegion> allocations; 
    FramedStorage<std::vector<ResourceRef<Buffer>>> retirementQueue;
    ResourceRef<Buffer> currentBuffer;
};