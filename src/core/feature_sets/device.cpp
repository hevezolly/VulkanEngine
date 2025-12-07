#include <device.h>
#include <common.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <set>
#include <vk_collect.h>
#include <present_feature.h>
#include <render_context.h>
#include <graphics_feature.h>

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
    case QueueType::Graphics:
        return ((properties->queueFlags) & VK_QUEUE_GRAPHICS_BIT) > 0;
    case QueueType::Present:
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

static bool TryConfigureQueueFamilies(
    QueueTypes queueTypes,
    VkPhysicalDevice device, 
    VkSurfaceKHR surface,
    QueueFamiliesDescriptor* descriptor
) {
   
    auto queueFamilies = vkCollect<VkQueueFamilyProperties>(vkGetPhysicalDeviceQueueFamilyProperties, device);

    std::vector<std::optional<uint32_t>> indices_vec((int)QueueType::None);
    for (QueueType type = (QueueType)0; (int)type < (int)QueueType::None; type = (QueueType)((int)type + 1)) {

        bool queueFits = false;
        if (((int)queueTypes & (1 << (int)type)) == 0) {
            continue;
        }

        for (uint32_t i = 0; i < queueFamilies.size(); i++) {
            if (CheckQueueFits(&queueFamilies[i], i, device, surface, type)) {
                indices_vec[(int)type] = i;
                queueFits = true;
                break;
            }
        }

        if (!queueFits)
            return false;
    }
    
    descriptor->queues = std::move(indices_vec);
    return true;
}

static bool SwapChainSupported(VkPhysicalDevice device, VkSurfaceKHR surface, SwapChainSupport* support) {
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &support->capabilities);
    support->surfaceFormats = std::move(vkCollect<VkSurfaceFormatKHR>(
        vkGetPhysicalDeviceSurfaceFormatsKHR, device, surface 
    ));

    support->presentModes = std::move(vkCollect<VkPresentModeKHR>(
        vkGetPhysicalDeviceSurfacePresentModesKHR, device, surface
    ));

    return support->surfaceFormats.size() > 0 && support->presentModes.size() > 0;
}

static bool DeviceSupportsExtentions(VkPhysicalDevice device) {

    auto availableExtensions = vkCollect<VkExtensionProperties>(
        vkEnumerateDeviceExtensionProperties, device, nullptr);

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

static VkPhysicalDevice FindDeviceOfType(
    VkPhysicalDevice* availableDevices, 
    uint32_t count, 
    QueueTypes queueTypes,
    VkPhysicalDeviceType type,
    VkSurfaceKHR surface,
    QueueFamiliesDescriptor* indices,
    SwapChainSupport* swapChainSupport
) {
    
    VkPhysicalDeviceProperties properties;

    for (uint32_t i = 0; i < count; ++i) {
        vkGetPhysicalDeviceProperties(availableDevices[i], &properties);

        if (properties.deviceType == type) {

            if (!TryConfigureQueueFamilies(queueTypes, availableDevices[i], surface, indices))
                continue;
            
            if ((queueTypes & (1 << (uint32_t)QueueType::Present)) > 0) {
                if (!DeviceSupportsExtentions(availableDevices[i]))
                    continue;
    
                if (!SwapChainSupported(availableDevices[i], surface, swapChainSupport))
                    continue;
            }
            
            return availableDevices[i];
        }
    }

    return VK_NULL_HANDLE;
}

static VkDevice CreateLogicalDevice(const QueueFamiliesDescriptor* queueDescriptors, VkPhysicalDevice physicalDevice) {

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies;
    float queuePriority = 1.f;
    
    for (std::optional<uint32_t> queueFamilyIndex: queueDescriptors->queues) {

        if (!queueFamilyIndex.has_value()) {
            continue;
        }

        if (uniqueQueueFamilies.count(queueFamilyIndex.value())) {
            continue;
        }

        uniqueQueueFamilies.insert(queueFamilyIndex.value());

        VkDeviceQueueCreateInfo queueCreateInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queueCreateInfo.queueFamilyIndex = queueFamilyIndex.value();
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    VkDeviceCreateInfo createInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(uniqueQueueFamilies.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    VkDevice device;
    VK(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device));
    return device;
}


void Device::Init() {

    VkInstance instance = context.vkInstance;

    auto availableDevices = vkCollect<VkPhysicalDevice>(vkEnumeratePhysicalDevices, instance);

    if (availableDevices.size() == 0) {
        throw std::runtime_error("no appropriate physical device is found");
    }

    PresentFeature* present = context.TryGet<PresentFeature>();

    QueueTypes queueTypes = 0;

    VkSurfaceKHR surface = VK_NULL_HANDLE;

    if (present != nullptr) {
        surface = present->window->vkSurface;
        queueTypes |= (1 << (int)QueueType::Present);
    }

    if (context.Has<GraphicsFeature>()) {
        queueTypes |= (1 << (int)QueueType::Graphics);
    }

    vkPhysicalDevice = FindDeviceOfType(availableDevices.data(), availableDevices.size(), queueTypes,
        VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, surface, &queueFamilies, &swapChainSupport);

    if (!vkPhysicalDevice) {
        vkPhysicalDevice = FindDeviceOfType(availableDevices.data(), availableDevices.size(), queueTypes,
            VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, surface, &queueFamilies, &swapChainSupport);
    }

    device = CreateLogicalDevice(&queueFamilies, vkPhysicalDevice);

    std::vector<std::optional<VkQueue>> preparedQueues(queueFamilies.queues.size());

    for (int i = 0; i < preparedQueues.size(); i++) {
        if (queueFamilies.queues[i].has_value()) {

            vkGetDeviceQueue(device, queueFamilies.queues[i].value(), 0, &preparedQueues[i].emplace(VK_NULL_HANDLE)); 
        }
    }

    queues.queues = std::move(preparedQueues);
}

void Device::Destroy() {
    if (device != VK_NULL_HANDLE) {
        vkDestroyDevice(device, nullptr);
        device = VK_NULL_HANDLE;
    }
}