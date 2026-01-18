#pragma once
#include <feature_set.h>
#include <resource_memory.h>
#include <buffer.h>
#include <handles.h>
#include <utility>
#include <image.h>
#include <image_usage.h>
#include <sampler.h>
#include <resource_id.h>
#include <unordered_map>
#include <resource_storage.h>

struct API Resources: FeatureSet,
    CanHandle<InitMsg>,
    CanHandle<DestroyMsg>
{
    using FeatureSet::FeatureSet;

    Memory AllocateMemory(VkMemoryRequirements requirements, VkMemoryPropertyFlags properties);

    ResourceRef<Buffer> CreateRawBuffer(BufferPreset preset, uint32_t size_bytes);
    
    ResourceRef<Buffer> CreateRawBuffer(BufferPreset preset, uint32_t size_bytes, void* data);
    
    template <typename T>
    ResourceRef<Buffer> CreateBuffer(BufferPreset preset, uint32_t elements_count, T* data) {
        return CreateRawBuffer(preset, elements_count * sizeof(T), data);
    }

    template <typename T>
    ResourceRef<Buffer> CreateBuffer(BufferPreset preset, uint32_t elementCount) {
        return CreateRawBuffer(preset, elementCount * sizeof(T));
    }
    
    template <typename T>
    ResourceRef<Buffer> CreateBuffer(BufferPreset preset, std::vector<T>& elements) {
        return CreateRawBuffer(preset, elements.size() * sizeof(T), elements.data());
    }

    ResourceRef<Sampler> CreateSampler(const SamplerFilter& filter, const SamplerAddressMode& addressMode);

    ResourceRef<Image> CreateImage(const ImageDescription& dsecription, ImageUsage usage);

    ResourceRef<Image> LoadImage(ImageUsage usage, const char* path, VkFormat format = VK_FORMAT_UNDEFINED);

    void DestroyImmediate(ResourceId resource);

    template <typename T>
    ResourceRef<T> Register(T&& value) {
        static_assert(std::is_same_v<T, Image> ||
                      std::is_same_v<T, Buffer> ||
                      std::is_same_v<T, Sampler>);
        return ResourceRef<T>();
    }

    template<>
    ResourceRef<Image> Resources::Register<Image>(Image&& img) {
        ResourceId id = _images.Insert(std::move(img));
        return {id, &_images};
    } 

    template<>
    ResourceRef<Buffer> Resources::Register<Buffer>(Buffer&& buffer) {
        ResourceId id = _buffers.Insert(std::move(buffer));
        return {id, &_buffers};
    } 

    template<>
    ResourceRef<Sampler> Resources::Register<Sampler>(Sampler&& sampler) {
        ResourceId id = _samplers.Insert(std::move(sampler));
        return {id, &_samplers};
    } 

    virtual void OnMessage(InitMsg*);
    virtual void OnMessage(DestroyMsg*);

private:
    VkPhysicalDeviceMemoryProperties vkMemProperties;

    ResourceStorage<Buffer> _buffers;
    ResourceStorage<Image> _images;
    ResourceStorage<Sampler> _samplers;
};
