#include "swap_chain.h"
#include <vk_collect.h>
#include <optional>
#include <iostream>
#include <limits>
#include <algorithm>
#include <render_context.h>
#include <present_feature.h>
#include <graphics_feature.h>
#include <resources.h>

static VkSurfaceFormatKHR SelectFormat(
    const SwapChainInitializer& args, 
    const std::vector<VkSurfaceFormatKHR>& availableFormats
) {
    for (auto desiredFormat: args.desiredFormats) {
        for (auto desiredColorSpace: args.desiredColorSpaces) {
            for (auto availableFormat: availableFormats) {
                if (desiredFormat == availableFormat.format &&
                    desiredColorSpace == availableFormat.colorSpace) {
                    return availableFormat;
                }
            }
        }
    }
#ifdef THROW_ON_UNDESIRED_SWAPCHAIN
    throw std::runtime_error("desired swapchain format is unavailable");
#endif
    return availableFormats[0];
}

static VkPresentModeKHR SelectPresentMode(
    const SwapChainInitializer& args,
    const std::vector<VkPresentModeKHR>& availablePresentModes
) {
    for (auto desiredMode: args.desiredPresentMode) {
        for (auto availableMode: availablePresentModes) {
            if (desiredMode == availableMode)
                return availableMode;
        }
    }
#ifdef THROW_ON_UNDESIRED_SWAPCHAIN
    throw std::runtime_error("desired swapchain present mode is unavailable");
#endif
    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D SelectExtents(const Window* window, const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.maxImageExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    int width, height;
    glfwGetFramebufferSize(window->pWindow, &width, &height);

    VkExtent2D extent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return extent;
}

static uint32_t SelectImageCount(uint32_t desiredImageCount, const VkSurfaceCapabilitiesKHR& capabilities) {
    
    uint32_t imageCount = desiredImageCount;
    if (imageCount == 0) {
        imageCount = capabilities.minImageCount + 1;
    }
    
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
#ifdef THROW_ON_UNDESIRED_SWAPCHAIN
        if (desiredImageCount == 0)
            throw std::runtime_error("desired swapchain image count is unsupported");
#endif
    }

    return imageCount;
}

static VkImageUsageFlagBits SelectUsage(ImageUsage desiredUsage, const VkSurfaceCapabilitiesKHR& capabilities) {

    VkImageUsageFlagBits desiredFlags = (VkImageUsageFlagBits)desiredUsage; 
    if (desiredFlags & capabilities.supportedUsageFlags != desiredFlags) {
        throw std::runtime_error("selected swapchain usage is unsupported");
    }

    return desiredFlags;
}

SwapChain::SwapChain(RenderContext* context, const SwapChainInitializer& args) {
    this->context = context;
    auto device = &context->Get<Device>();
    auto window = context->Get<PresentFeature>().window;
    SwapChainSupport& support = context->Get<PresentFeature>().swapChainSupport;
    VkSurfaceCapabilitiesKHR& capabilities = support.capabilities;

    VkSurfaceFormatKHR selectedFormat = SelectFormat(args, support.surfaceFormats);
    format = selectedFormat.format;
    colorSpace = selectedFormat.colorSpace;

    presentMode = SelectPresentMode(args, support.presentModes);
    extent = SelectExtents(window, capabilities);
    uint32_t imageCount = SelectImageCount(args.imageCount, capabilities);
    imageUsage = args.imageUsage;
    VkImageUsageFlagBits usage = SelectUsage(args.imageUsage, capabilities);

    VkSwapchainCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};

    createInfo.surface = window->vkSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = selectedFormat.format;
    createInfo.imageColorSpace = selectedFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageUsage = usage;
    createInfo.imageArrayLayers = 1;
    createInfo.presentMode = presentMode;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    uint32_t usedQueuesCount = 1;
    uint32_t usedQueuesIndices[2];
    usedQueuesIndices[0] = device->queueFamilies.get(QueueType::Present);

    if (context->Has<GraphicsFeature>()) {
        usedQueuesIndices[usedQueuesCount++] = device->queueFamilies.get(QueueType::Graphics);
    }

    if (usedQueuesCount == 1 || usedQueuesIndices[0] == usedQueuesIndices[1]) {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = usedQueuesIndices;
    }

    VK(vkCreateSwapchainKHR(device->device, &createInfo, nullptr, &swapChain));

    auto premadeImages = vkCollect<VkImage>(vkGetSwapchainImagesKHR, device->device, swapChain);

    ImageDescription description{};
    description.depth = 1;
    description.width = extent.width;
    description.height = extent.height;
    description.format = format;
    
    for (auto img: premadeImages) {
        images.push_back(context->Get<Resources>().Register(Image(img, *context, description)));
    }
}

SwapChain& SwapChain::operator=(SwapChain&& other) noexcept {
    if (this == &other)
        return *this;

    images = std::move(other.images);
    swapChain = other.swapChain;
    extent = other.extent;
    format = other.format;
    colorSpace = other.colorSpace;
    imageUsage = other.imageUsage;
    context = other.context;

    other.swapChain = VK_NULL_HANDLE;
    other.context = nullptr;

    return *this;
}

SwapChain::SwapChain(SwapChain&& other) noexcept {
    *this = std::move(other);
}

SwapChain::~SwapChain() {
    if (context) {
        
        Resources& r = context->Get<Resources>();
        for (auto image: images) {
            r.DestroyImmediate(image);
        }
        images.clear();

        vkDestroySwapchainKHR(context->device(), swapChain, nullptr);
    }
}

