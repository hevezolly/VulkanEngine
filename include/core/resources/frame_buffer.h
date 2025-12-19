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

    RULE_5(FrameBuffer)

private: 
    RenderContext* context;
};
