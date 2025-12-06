#pragma once
#include <vector>
#include <volk.h>
#include <common.h>
#include <window.h>
#include <device.h>
#include <image.h>

struct API SwapChainInitializer {
    std::vector<VkFormat> desiredFormats = {VK_FORMAT_B8G8R8A8_SRGB};
    std::vector<VkColorSpaceKHR> desiredColorSpaces = {VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    std::vector<VkPresentModeKHR> desiredPresentMode = {VK_PRESENT_MODE_FIFO_KHR};
    ImageUsage imageUsage = ImageUsage::ColorAttachment;
    uint32_t imageCount = 0;
};

struct API SwapChain
{
    VkSwapchainKHR swapChain;

    VkExtent2D extent;
    VkFormat format;
    VkColorSpaceKHR colorSpace;
    VkPresentModeKHR presentMode;
    ImageUsage imageUsage;
    std::vector<Image> images;
    

    SwapChain(Window* window, Device* device, const SwapChainInitializer& args);

    RULE_5(SwapChain)

private: 
    Device* device;
};
