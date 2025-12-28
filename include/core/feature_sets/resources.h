#pragma once
#include <feature_set.h>
#include <resource_memory.h>
#include <buffer.h>
#include <handles.h>
#include <utility>
#include <image.h>
#include <image_usage.h>
#include <sampler.h>

struct API Resources: FeatureSet,
    CanHandle<InitMsg>
{
    using FeatureSet::FeatureSet;

    Memory AllocateMemory(VkMemoryRequirements requirements, VkMemoryPropertyFlags properties);

    Buffer CreateRawBuffer(BufferPreset preset, uint32_t size_bytes);
    
    Buffer CreateRawBuffer(BufferPreset preset, uint32_t size_bytes, void* data);
    
    template <typename T>
    Ref<Buffer> CreateBuffer(BufferPreset preset, uint32_t elements_count, T* data) {
        return newBuffer(std::move(CreateRawBuffer(preset, elements_count * sizeof(T), data)));
    }

    template <typename T>
    Ref<Buffer> CreateBuffer(BufferPreset preset, uint32_t elementCount) {
        return newBuffer(std::move(CreateRawBuffer(preset, elementCount * sizeof(T))));
    }
    
    template <typename T>
    Ref<Buffer> CreateBuffer(BufferPreset preset, std::vector<T>& elements) {
        return newBuffer(std::move(CreateRawBuffer(preset, elements.size() * sizeof(T), elements.data())));
    }

    Image CreateRawImage(const ImageDescription& dsecription, ImageUsage usage);

    Image LoadRawImage(ImageUsage usage, const char* path, VkFormat format = VK_FORMAT_UNDEFINED);

    Sampler CreateRawSampler(const SamplerFilter& filter, const SamplerAddressMode& addressMode);

    Ref<Sampler> CreateSampler(const SamplerFilter& filter, const SamplerAddressMode& addressMode);

    Ref<Image> CreateImage(const ImageDescription& dsecription, ImageUsage usage);

    Ref<Image> LoadImage(ImageUsage usage, const char* path, VkFormat format = VK_FORMAT_UNDEFINED);

    virtual void OnMessage(InitMsg*);

private:
    Ref<Buffer> newBuffer(Buffer&&);
    VkPhysicalDeviceMemoryProperties vkMemProperties;
};
