#pragma once
#include <render_context.h>
#include <common.h>
#include <image.h>

struct API FrameBuffer
{
    uint32_t width;
    uint32_t height;
    uint32_t size;
    VkFramebuffer frameBuffer;
    std::vector<VkClearValue> clearValues;
    FrameBuffer(RenderContext*, VkRenderPass pass, VkImageView* vkView, VkClearValue* clearValues, uint32_t count, uint32_t width, uint32_t height);

    RULE_5(FrameBuffer)

private: 
    RenderContext* context;
};
