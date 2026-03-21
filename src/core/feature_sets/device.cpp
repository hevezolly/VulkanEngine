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

static bool CheckQueueFits(
    VkQueueFamilyProperties* properties, 
    uint32_t familyIndex,
    VkPhysicalDevice device, 
    VkSurfaceKHR surface,
    QueueType type,
    bool& suboptimal
) {
    suboptimal = false;
    switch (type)
    {
    case QueueType::Graphics:
        return ((properties->queueFlags) & (VK_QUEUE_GRAPHICS_BIT)) > 0;
    case QueueType::Present:
        VkBool32 support;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, familyIndex, surface, &support);
        return support == VK_TRUE;
    case QueueType::Transfer:
        suboptimal = ((properties->queueFlags) & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) > 0;
        return (((properties->queueFlags) & (VK_QUEUE_TRANSFER_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) > 0);
    case QueueType::Compute:
        suboptimal = ((properties->queueFlags) & (VK_QUEUE_GRAPHICS_BIT)) > 0;
        return (properties->queueFlags) & (VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT) > 0;
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
            bool suboptimalQueue = false;
            if (CheckQueueFits(&queueFamilies[i], i, device, surface, type, suboptimalQueue)) {
                indices_vec[(int)type] = i;
                queueFits = true;

                if (!suboptimalQueue)
                    break;
            }
        }

        if (!queueFits)
            return false;
    }
    
    descriptor->queues = std::move(indices_vec);
    return true;
}

static bool DeviceSupportsExtentions(VkPhysicalDevice device, std::vector<const char*>& deviceExtensions) {

    auto availableExtensions = vkCollect<VkExtensionProperties>(
        vkEnumerateDeviceExtensionProperties, device, nullptr);

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

static VkPhysicalDevice FindDeviceOfType(
    RenderContext& context,
    VkPhysicalDevice* availableDevices, 
    uint32_t count, 
    QueueTypes queueTypes,
    VkPhysicalDeviceType type,
    VkSurfaceKHR surface,
    QueueFamiliesDescriptor* indices,
    std::vector<const char*>& extensions
) {
    
    VkPhysicalDeviceProperties properties;

    for (uint32_t i = 0; i < count; ++i) {
        vkGetPhysicalDeviceProperties(availableDevices[i], &properties);

        if (properties.deviceType == type) {

            if (!TryConfigureQueueFamilies(queueTypes, availableDevices[i], surface, indices))
                continue;
            
            if (!DeviceSupportsExtentions(availableDevices[i], extensions))
                continue;

            CheckDeviceAppropriateMsg deviceAppropriate(availableDevices[i]);
            context.Send(&deviceAppropriate);
            
            if (!deviceAppropriate.appropriate)
                continue;
            
            return availableDevices[i];
        }
    }

    LOG("NOT FOUND!")

    return VK_NULL_HANDLE;
}

static VkDevice CreateLogicalDevice(
    RenderContext& context,
    const QueueFamiliesDescriptor* queueDescriptors, 
    VkPhysicalDevice physicalDevice,
    std::vector<const char*> deviceExtensions,
    QueuesDescriptor<VkQueue>& queues
) {

    Allocator& alloc = context.Get<Allocator>();
    auto _ = alloc.BeginContext();

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::unordered_map<uint32_t, uint32_t> queuesCountPerFamily;
    
    for (std::optional<uint32_t> queueFamilyIndex: queueDescriptors->queues) {

        if (!queueFamilyIndex.has_value()) {
            continue;
        }

        queuesCountPerFamily[queueFamilyIndex.value()]++;
    }

    for (auto& pair : queuesCountPerFamily) {

        MemChunk<float> priorities = alloc.BumpAllocate<float>(pair.second);
        for (int i = 0; i < pair.second; i++) {
            priorities[i] = 1.0f;
        }

        VkDeviceQueueCreateInfo queueCreateInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queueCreateInfo.queueFamilyIndex = pair.first;
        queueCreateInfo.queueCount = pair.second;
        queueCreateInfo.pQueuePriorities = priorities.data;
    
        queueCreateInfos.push_back(queueCreateInfo);
    }


    VkPhysicalDeviceFeatures deviceFeatures{};

    VkPhysicalDeviceVulkan13Features vulkan13Features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    vulkan13Features.synchronization2 = VK_TRUE;
    
    VkPhysicalDeviceVulkan12Features vk12Features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    vk12Features.pNext = &vulkan13Features;
    vk12Features.timelineSemaphore = VK_TRUE;

    VkDeviceCreateInfo createInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    createInfo.pNext = &vk12Features;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    VkDevice device;
    VK(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device));

    queuesCountPerFamily.clear();

    queues.queues.resize(queueDescriptors->queues.size());
    for (int i = 0; i < queueDescriptors->queues.size(); i++) {
        if (!queueDescriptors->queues[i]) {
            continue;
        }

        uint32_t family = queueDescriptors->queues[i].value();
        uint32_t index = queuesCountPerFamily[family]++;

        vkGetDeviceQueue(device, family, index, &queues.queues[i].emplace(VK_NULL_HANDLE));
    }

    return device;
}


void Device::OnMessage(InitMsg* m) {

    VkInstance instance = context.vkInstance;

    auto availableDevices = vkCollect<VkPhysicalDevice>(vkEnumeratePhysicalDevices, instance);

    if (availableDevices.size() == 0) {
        throw std::runtime_error("no appropriate physical device is found");
    }

    PresentFeature* present = context.TryGet<PresentFeature>();

    CollectRequiredQueueTypesMsg queuesM{};
    queuesM.requiredTypes |= QueueType::Transfer;

    context.Send(&queuesM);
    QueueTypes queueTypes = queuesM.requiredTypes;

    std::vector<const char*> deviceExtensions{};

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (context.Has<PresentFeature>()) {
        surface = context.Get<PresentFeature>().window->vkSurface;
    }

    context.Send(CollectDeviceRequirementsMsg{&deviceExtensions});

    vkPhysicalDevice = FindDeviceOfType(context, availableDevices.data(), availableDevices.size(), queueTypes,
        VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, 
        surface, &queueFamilies, deviceExtensions);

    if (!vkPhysicalDevice) {
        vkPhysicalDevice = FindDeviceOfType(context, availableDevices.data(), availableDevices.size(), queueTypes,
            VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, 
            surface, &queueFamilies, deviceExtensions);
    }
    
    device = CreateLogicalDevice(context, &queueFamilies, vkPhysicalDevice, deviceExtensions, queues);
}

void Device::OnMessage(DestroyMsg* m) {
    if (device != VK_NULL_HANDLE) {
        vkDestroyDevice(device, nullptr);
        device = VK_NULL_HANDLE;
    }
}

MemBuffer<uint32_t> Device::FillQueueUsages(QueueTypes types) {
    MemBuffer<uint32_t> data = context.Get<Allocator>().BumpAllocate<uint32_t>(static_cast<uint32_t>(QueueType::None));
    for (QueueType type = (QueueType)0; (int)type < (int)QueueType::None; type = (QueueType)((int)type + 1)) {
        if ((types & type) == 0)
            continue;

        std::optional<uint32_t> queue = queueFamilies.try_get(type);
        if (!queue.has_value())
            continue;

        if (std::find(data.begin(), data.end(), queue.value()) != data.end())
            continue;

        data.push_back(queue.value());
    }
    return data;
}

bool Device::CheckFormatSupported(VkFormat format, VkImageTiling tiling, VkFormatFeatureFlags features) {
    auto it = formatSupport.find(format);
    if (it == formatSupport.end()) {

        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(vkPhysicalDevice, format, &props);
        formatSupport[format] = {props.linearTilingFeatures, props.optimalTilingFeatures};
        
        it = formatSupport.find(format);
    }

    if (tiling == VK_IMAGE_TILING_LINEAR)
        return (it->second.first & features) == features;

    if (tiling == VK_IMAGE_TILING_OPTIMAL)
        return (it->second.second & features) == features;

    return false;
}

VkFormat Device::SelectSupportedFormat(std::initializer_list<VkFormat> formats, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (auto format : formats) {
        if (CheckFormatSupported(format, tiling, features))
            return format;
    }

    return VK_FORMAT_UNDEFINED;
}