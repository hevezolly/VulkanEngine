#include "device.h"
#include <common.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <set>

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static bool CheckQueueFits(
    VkQueueFamilyProperties* properties, 
    uint32_t familyIndex,
    VkPhysicalDevice device, 
    VkSurfaceKHR surface,
    QueueType type
) {
    switch (type)
    {
    case Graphics:
        return ((properties->queueFlags) & VK_QUEUE_GRAPHICS_BIT) > 0;
    case Present:
        VkBool32 support;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, familyIndex, surface, &support);
        return support == VK_TRUE;
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

static bool TryConfigureQueueFamilies(
    VkPhysicalDevice device, 
    VkSurfaceKHR surface,
    QueueFamiliesDescriptor* descriptor
) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    std::vector<uint32_t> indices_vec;
    for (QueueType type = (QueueType)0; (int)type < (int)QueueType::None; type = (QueueType)((int)type + 1)) {
        for (uint32_t i = 0; i < queueFamilyCount; i++) {
            if (CheckQueueFits(&queueFamilies[i], i, device, surface, type)) {
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

static bool DeviceSupportsExtentions(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

static VkPhysicalDevice FindDeviceOfType(
    VkPhysicalDevice* availableDevices, 
    uint32_t count, 
    VkPhysicalDeviceType type,
    VkSurfaceKHR surface,
    QueueFamiliesDescriptor* indices
) {
    
    VkPhysicalDeviceProperties properties;

    for (uint32_t i = 0; i < count; ++i) {
        vkGetPhysicalDeviceProperties(availableDevices[i], &properties);

        if (properties.deviceType == type) {

            if (!DeviceSupportsExtentions(availableDevices[i]))
                continue;

            if (!TryConfigureQueueFamilies(availableDevices[i], surface, indices))
                continue;
            
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
    createInfo.enabledLayerCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    VkDevice device;
    VK(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device));
    return device;
}

Device::Device(VkInstance instance, VkSurfaceKHR surface) {
    uint32_t count;
    VK(vkEnumeratePhysicalDevices(instance, &count, nullptr));

    if (count == 0) {
        throw std::runtime_error("no physical devices are found");
    }
    
    VkPhysicalDevice* availableDevices = new VkPhysicalDevice[count];

    VK(vkEnumeratePhysicalDevices(instance, &count, availableDevices));

    vkPhysicalDevice = FindDeviceOfType(availableDevices, count, 
        VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, surface, &queueFamilies);

    if (!vkPhysicalDevice)
        vkPhysicalDevice = FindDeviceOfType(availableDevices, count, 
        VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, surface, &queueFamilies);

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