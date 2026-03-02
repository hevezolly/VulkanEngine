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
#include <synchronization.h>

struct API Resources: FeatureSet,
    CanHandle<InitMsg>,
    CanHandle<DestroyMsg>
{
    using FeatureSet::FeatureSet;

    Memory AllocateMemory(VkMemoryRequirements requirements, VkMemoryPropertyFlags properties, uint32_t usable_size = 0);

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
    ResourceRef<T> Register(T&& value, const ResourceState& resourceState) {
        static_assert(std::is_same_v<T, Image> ||
                      std::is_same_v<T, Buffer>);
        return ResourceRef<T>();
    }

    template<>
    ResourceRef<Image> Resources::Register<Image>(Image&& img, const ResourceState& resourceState) {
        ResourceId id = _images.Insert(std::move(img));
        _states[id] = resourceState;
        return {id, &_images};
    } 

    template<>
    ResourceRef<Buffer> Resources::Register<Buffer>(Buffer&& buffer, const ResourceState& resourceState) {
        ResourceId id = _buffers.Insert(std::move(buffer));
        _states[id] = resourceState;
        return {id, &_buffers};
    }

    template <typename T>
    ResourceRef<T> Get(ResourceId id) {
        static_assert(std::is_same_v<T, Image> ||
                      std::is_same_v<T, Buffer> ||
                      std::is_same_v<T, Sampler>);
        return ResourceRef<T>();
    }

    template<>
    ResourceRef<Image> Resources::Get<Image>(ResourceId id) {
        assert(id.type() == ResourceType::Image);
        return {id, &_images};
    } 

    template<>
    ResourceRef<Buffer> Resources::Get<Buffer>(ResourceId id) {
        assert(id.type() == ResourceType::Buffer);
        return {id, &_buffers};
    }

    template<>
    ResourceRef<Sampler> Resources::Get<Sampler>(ResourceId id) {
        assert(id.type() == ResourceType::Sampler);
        return {id, &_samplers};
    }

    void GiveName(ResourceId id, const std::string& name);

    const std::string& GetName(ResourceId id);

    ResourceState GetState(ResourceId id);
    void SetState(ResourceId id, const ResourceState& state);
    ResourceState UpdateState(ResourceId id, const ResourceState& state);

    virtual void OnMessage(InitMsg*);
    virtual void OnMessage(DestroyMsg*);

    bool ResourceRequiresSynchronization(ResourceId resource);
    Ref<Semaphore> ExtractSyncContext(ResourceId resource);
    void SetSynchronizationContext(ResourceId resource, Ref<Semaphore>);

private:
    VkPhysicalDeviceMemoryProperties vkMemProperties;

    std::unordered_map<ResourceId, ResourceState> _states;
    std::unordered_map<ResourceId, std::string> _names;
    std::unordered_map<ResourceId, Ref<Semaphore>> _synchronization;
    
    ResourceStorage<Buffer> _buffers;
    ResourceStorage<Image> _images;
    ResourceStorage<Sampler> _samplers;
};
