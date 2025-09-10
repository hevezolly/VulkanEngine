#pragma once

#include <volk.h>
#include <vector>
#include <optional>

enum struct QueueType {
    Graphics,
    Present,
    None
};

typedef uint32_t QueueTypes;

template<typename T>
struct QueuesDescriptor {
    std::vector<std::optional<T>> queues;    
    T get(QueueType);
};

typedef QueuesDescriptor<uint32_t> QueueFamiliesDescriptor;

struct SwapChainSupport {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct Device {
    VkPhysicalDevice vkPhysicalDevice;
    VkDevice device;
    QueueFamiliesDescriptor queueFamilies;
    QueuesDescriptor<VkQueue> queues;
    SwapChainSupport swapChainSupport;

    Device(VkInstance instance, QueueTypes queueTypes, VkSurfaceKHR surface);
    ~Device();
};