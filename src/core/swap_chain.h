#pragma once
#include <vector>
#include <volk.h>
#include <common.h>
#include <window.h>
#include <device.h>

struct SwapChainInitializer {
    std::vector<VkFormat> desiredFormats = {VK_FORMAT_B8G8R8A8_SRGB};
    std::vector<VkColorSpaceKHR> desiredColorSpace = {VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    std::vector<VkPresentModeKHR> desiredPresentMode = {VK_PRESENT_MODE_FIFO_KHR};
    ImageUsage imageUsage = ImageUsage::ColorAttachment;
    uint32_t imageCount = 0;
};

struct SwapChain
{
    uint32_t withd;
    uint32_t height;

    VkFormat format;
    VkColorSpaceKHR colorSpace;
    VkPresentModeKHR presentMode;
    uint32_t imageCount;

    SwapChain(Window* window, Device* device, SwapChainInitializer* args);
    ~SwapChain();
};
