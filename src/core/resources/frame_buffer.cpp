#include <frame_buffer.h>

FrameBuffer::FrameBuffer(RenderContext* context, ImageView* image, VkRenderPass renderPass)
{
    this->context = context;

    VkFramebufferCreateInfo framebufferInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &image->vkImageView;
    framebufferInfo.width = image->referencedImage->width;
    framebufferInfo.height = image->referencedImage->height;
    framebufferInfo.layers = 1;
    framebufferInfo.flags = 0;
    width = image->referencedImage->width;
    height = image->referencedImage->height;

    
    VK(vkCreateFramebuffer(context->device(), &framebufferInfo, nullptr, &frameBuffer))
}

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