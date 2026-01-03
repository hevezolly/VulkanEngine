#include <frame_buffer.h>

FrameBuffer::FrameBuffer(
    RenderContext* context,
    VkRenderPass pass, 
    VkImageView* vkView, 
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

    
    VK(vkCreateFramebuffer(context->device(), &framebufferInfo, nullptr, &frameBuffer))
}

FrameBuffer::FrameBuffer(RenderContext* context, ImageView* image, VkRenderPass renderPass):
    FrameBuffer(context, renderPass, &image->vkImageView, static_cast<uint32_t>(1), image->referencedImage->description.width, image->referencedImage->description.height)
{}

FrameBuffer& FrameBuffer::operator=(FrameBuffer&& other) noexcept {
    if (this == &other)
        return *this;

    context = other.context;
    frameBuffer = other.frameBuffer;
    width = other.width;
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