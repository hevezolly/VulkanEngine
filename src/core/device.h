#pragma once

#include <volk.h>
#include <vector>

enum QueueType {
    Graphics,
    Present,
    None
};


template<typename T>
struct QueuesDescriptor {
    std::vector<T> queues;    
    T get(QueueType);
};

typedef QueuesDescriptor<uint32_t> QueueFamiliesDescriptor;

struct Device {
    VkPhysicalDevice vkPhysicalDevice;
    VkDevice device;
    QueueFamiliesDescriptor queueFamilies;
    QueuesDescriptor<VkQueue> queues;

    Device(VkInstance instance, VkSurfaceKHR surface);
    ~Device();
};