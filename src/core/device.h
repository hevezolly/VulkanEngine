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

template<typename T>
T QueuesDescriptor<T>::get(QueueType type) {
    std::optional<T>* value = &queues[(int)type];
    if (!value->has_value()) {
        std::stringstream ss;
        ss << "queue type " << (int)type << " is not initialized";
        throw std::invalid_argument(ss.str());
    }
    return value->value();
}

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
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    Device(Device&&) noexcept;
    Device& operator=(Device&&) noexcept;
   
    ~Device();
};