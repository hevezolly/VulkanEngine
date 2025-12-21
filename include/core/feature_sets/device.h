#pragma once

#include <volk.h>
#include <vector>
#include <optional>
#include <common.h>
#include <feature_set.h>
#include <messages.h>

template<typename T>
struct QueuesDescriptor {
    std::vector<std::optional<T>> queues;    
    T get(QueueType);
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

typedef QueuesDescriptor<uint32_t> QueueFamiliesDescriptor;

struct API Device: FeatureSet, 
    CanHandle<InitMsg>,
    CanHandle<DestroyMsg>
{
    
    VkPhysicalDevice vkPhysicalDevice;
    VkDevice device;
    QueueFamiliesDescriptor queueFamilies;
    QueuesDescriptor<VkQueue> queues;

    using FeatureSet::FeatureSet;
    virtual void OnMessage(InitMsg*);
    virtual void OnMessage(DestroyMsg*);
};