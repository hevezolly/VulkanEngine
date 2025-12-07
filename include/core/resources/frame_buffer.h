#pragma once
#include <render_context.h>
#include <common.h>
#include <image.h>

struct FrameBuffer
{
    VkFramebuffer frameBuffer;
    FrameBuffer(RenderContext* context, uint32_t imageCount, ImageView* images, VkRenderPass renderPass);

    RULE_5(FrameBuffer)

private: 
    RenderContext* context;
};
