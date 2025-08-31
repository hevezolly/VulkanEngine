#include "device.h"
#include <common.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <set>

static VkQueueFlagBits QueueTypeToFlags(QueueType type) {
    switch (type)
    {
    case Graphics:
        return VK_QUEUE_GRAPHICS_BIT;
        break;
    
    default:
        std::stringstream ss;
        ss << "queue type " << (int)type << " is invalid";
        throw std::invalid_argument(ss.str());
        break;
    }
}

template<typename T>
T QueuesDescriptor<T>::get(QueueType type) {
    return queues[(int)type];
}

static bool TryConfigureDescriptor(VkPhysicalDevice device, QueueFamiliesDescriptor* descriptor) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    std::vector<uint32_t> indices_vec;
    for (QueueType type = (QueueType)0; (int)type < (int)QueueType::None; type = (QueueType)((int)type + 1)) {
        VkQueueFlagBits flag = QueueTypeToFlags(type);

        for (uint32_t i = 0; i < queueFamilyCount; i++) {
            if (queueFamilies[i].queueFlags & flag) {
                indices_vec.push_back(i);
                break;
            }
        }
    }

    if (indices_vec.size() != (int)QueueType::None)
        return false;
    
    descriptor->queues = std::move(indices_vec);
    return true;
}

static VkPhysicalDevice FindDeviceWithType(
    VkPhysicalDevice* availableDevices, 
    uint32_t count, 
    VkPhysicalDeviceType type,
    QueueFamiliesDescriptor* indices
) {
    
    VkPhysicalDeviceProperties properties;

    for (uint32_t i = 0; i < count; ++i) {
        vkGetPhysicalDeviceProperties(availableDevices[i], &properties);

        if (properties.deviceType == type) {

            if (TryConfigureDescriptor(availableDevices[i], indices))
                return availableDevices[i];
        }
    }

    return VK_NULL_HANDLE;
}

static VkDevice CreateLogicalDevice(const QueueFamiliesDescriptor* queueDescriptors, VkPhysicalDevice physicalDevice) {

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies(queueDescriptors->queues.begin(), queueDescriptors->queues.end());
    float queuePriority = 1.f;
    
    for (uint32_t queueFamilyIndex: uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    VkDeviceCreateInfo createInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(uniqueQueueFamilies.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledLayerCount = 0;

    VkDevice device;
    VK(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device));
    return device;
}

Device::Device(VkInstance instance) {
    uint32_t count;
    VK(vkEnumeratePhysicalDevices(instance, &count, nullptr));

    if (count == 0) {
        throw std::runtime_error("no physical devices are found");
    }
    
    VkPhysicalDevice* availableDevices = new VkPhysicalDevice[count];

    VK(vkEnumeratePhysicalDevices(instance, &count, availableDevices));

    vkPhysicalDevice = FindDeviceWithType(availableDevices, count, 
        VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, &queueFamilies);

    if (!vkPhysicalDevice)
        vkPhysicalDevice = FindDeviceWithType(availableDevices, count, 
        VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, &queueFamilies);

    delete[] availableDevices;
    if (count == 0) {
        throw std::runtime_error("no appropriate physical device is found");
    }

    device = CreateLogicalDevice(&queueFamilies, vkPhysicalDevice);

    std::vector<VkQueue> preparedQueues(queueFamilies.queues.size());

    for (int i = 0; i < preparedQueues.size(); i++) {
        vkGetDeviceQueue(device, queueFamilies.queues[i], 0, &preparedQueues[i]); 
    }

    queues.queues = std::move(preparedQueues);
}

Device::~Device() {
    vkDestroyDevice(device, nullptr);
}