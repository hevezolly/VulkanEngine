#pragma once

#include <common.h>
#include <volk.h>
#include <vector>
#include <optional>
#include <feature_set.h>
#include <messages.h>
#include <allocator_feature.h>
#include <unordered_map>

template<typename T>
struct QueuesDescriptor {
    std::vector<std::optional<T>> queues;    
    T get(QueueType);
    std::optional<T> try_get(QueueType);
};

template<typename T>
T QueuesDescriptor<T>::get(QueueType type) {
    if ((int)type >= queues.size()) {
        std::stringstream ss;
        ss << "queue type " << (int)type << " is not initialized";
        throw std::invalid_argument(ss.str());
    }

    std::optional<T>* value = &queues[(int)type];
    if (!value->has_value()) {
        std::stringstream ss;
        ss << "queue type " << (int)type << " is not initialized";
        throw std::invalid_argument(ss.str());
    }
    return value->value();
}

template<typename T>
std::optional<T> QueuesDescriptor<T>::try_get(QueueType type) {
   if ((int)type >= queues.size()) {
        return std::nullopt;
    }

    std::optional<T>* value = &queues[(int)type];
    if (!value->has_value()) {
        return std::nullopt;
    }
    return *value;
}

typedef QueuesDescriptor<uint32_t> QueueFamiliesDescriptor;

struct API Device: FeatureSet, 
    CanHandle<InitMsg>,
    CanHandle<DestroyMsg>
{
    
    VkPhysicalDevice vkPhysicalDevice;
    VkDevice device;
    QueueFamiliesDescriptor queueFamilies;
    QueuesDescriptor<VkQueue> queues;

    MemBuffer<uint32_t> FillQueueUsages(QueueTypes tupes);

    using FeatureSet::FeatureSet;
    virtual void OnMessage(InitMsg*);
    virtual void OnMessage(DestroyMsg*);

    bool CheckFormatSupported(VkFormat format, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat SelectSupportedFormat(std::initializer_list<VkFormat>, VkImageTiling tiling, VkFormatFeatureFlags features);

private: 
    std::unordered_map<VkFormat, std::pair<VkFormatFeatureFlags, VkFormatFeatureFlags>> formatSupport;
};