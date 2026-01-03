#pragma once
#include <render_context.h>
#include <common.h>
#include <image.h>

#define FrameBuffer_CtorArgs ImageView*, VkRenderPass

struct API FrameBuffer
{
    uint32_t width;
    uint32_t height;
    VkFramebuffer frameBuffer;
    FrameBuffer(RenderContext*, FrameBuffer_CtorArgs);
    FrameBuffer(RenderContext*, VkRenderPass pass, VkImageView* vkView, uint32_t count, uint32_t width, uint32_t height);

    RULE_5(FrameBuffer)

private: 
    RenderContext* context;
};
