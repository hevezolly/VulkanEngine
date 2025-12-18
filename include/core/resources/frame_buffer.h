#pragma once
#include <render_context.h>
#include <common.h>
#include <image.h>

struct API FrameBuffer
{
    uint32_t width;
    uint32_t height;
    VkFramebuffer frameBuffer;
    FrameBuffer(RenderContext* context, ImageView* image, VkRenderPass renderPass);

    RULE_5(FrameBuffer)

private: 
    RenderContext* context;
};
