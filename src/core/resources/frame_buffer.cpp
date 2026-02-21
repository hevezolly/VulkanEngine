#include <frame_buffer.h>
#include <render_context.h>

FrameBuffer::FrameBuffer(
    RenderContext* context,
    VkRenderPass pass, 
    VkImageView* vkView,
    VkClearValue* vkClearVals,
    uint32_t count, 
    uint32_t width, 
    uint32_t height
) {
     this->context = context;

    VkFramebufferCreateInfo framebufferInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    framebufferInfo.renderPass = pass;
    framebufferInfo.attachmentCount = count;
    framebufferInfo.pAttachments = vkView;
    framebufferInfo.width = width;
    framebufferInfo.height = height;
    framebufferInfo.layers = 1;
    framebufferInfo.flags = 0;
    this->width = width;
    this->height = height;
    this->size = count;
    clearValues.reserve(count);
    for (int i = 0; i < count; i++) {
        clearValues.push_back(vkClearVals[i]);
    }
        
    VK(vkCreateFramebuffer(context->device(), &framebufferInfo, nullptr, &frameBuffer))
}

FrameBuffer& FrameBuffer::operator=(FrameBuffer&& other) noexcept {
    if (this == &other)
        return *this;

    clearValues = std::move(other.clearValues);
    context = other.context;
    frameBuffer = other.frameBuffer;
    width = other.width;
    size = other.size;
    height = other.height;
    other.context = nullptr;
    other.frameBuffer = VK_NULL_HANDLE;

    return *this;    
}

FrameBuffer::FrameBuffer(FrameBuffer&& other) noexcept {
    *this = std::move(other);
}

FrameBuffer::~FrameBuffer() {
    if (context) {
        vkDestroyFramebuffer(context->device(), frameBuffer, nullptr);
        context = nullptr;
    }
}