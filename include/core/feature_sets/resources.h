#pragma once
#include <feature_set.h>
#include <resource_memory.h>
#include <buffer.h>
#include <handles.h>
#include <utility>

struct API Resources: FeatureSet,
    CanHandle<InitMsg>
{
    using FeatureSet::FeatureSet;

    Memory AllocateMemory(VkMemoryRequirements requirements, VkMemoryPropertyFlags properties);

    Buffer CreateRawBuffer(BufferPreset preset, uint32_t size_bytes);
    
    Buffer CreateRawBuffer(BufferPreset preset, uint32_t size_bytes, void* data);
    
    template <typename T>
    Ref<Buffer> CreateBuffer(BufferPreset preset, uint32_t elements_count, T* data) {
        return context.New<Buffer>(std::move(CreateRawBuffer(preset, elements_count * sizeof(T), data)));
    }

    template <typename T>
    Ref<Buffer> CreateBuffer(BufferPreset preset, uint32_t elementCount) {
        return context.New<Buffer>(std::move(CreateRawBuffer(preset, elementCount * sizeof(T))));
    }
    
    template <typename T>
    Ref<Buffer> CreateBuffer(BufferPreset preset, std::vector<T>& elements) {
        return context.New<Buffer>(std::move(CreateRawBuffer(preset, elements.size() * sizeof(T), elements.data())));
    }


    virtual void OnMessage(InitMsg*);

private:
    VkPhysicalDeviceMemoryProperties vkMemProperties;
};
