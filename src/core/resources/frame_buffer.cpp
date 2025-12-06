#include <frame_buffer.h>

FrameBuffer::FrameBuffer(RenderContext* context, uint32_t imageCount, ImageView* images, VkRenderPass renderPass)
{
    VkImageView* attachments = new VkImageView[imageCount];

    for (int i = 0; i < imageCount; i++) {
        attachments[i] = images[i].vkImageView;
    }

    this->context = context;

    VkFramebufferCreateInfo framebufferInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = imageCount;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = images[0].referencedImage->width;
    framebufferInfo.height = images[0].referencedImage->height;
    framebufferInfo.layers = 1;

    
    VK(vkCreateFramebuffer(context->device->device, &framebufferInfo, nullptr, &frameBuffer))
}

FrameBuffer& FrameBuffer::operator=(FrameBuffer&& other) {
    if (this == &other)
        return *this;

    context = other.context;
    frameBuffer = other.frameBuffer;
    other.context = nullptr;
    other.frameBuffer = VK_NULL_HANDLE;

    return *this;
}

FrameBuffer::FrameBuffer(FrameBuffer&& other) {
    *this = std::move(other);
}

FrameBuffer::~FrameBuffer() {
    if (context) {
        vkDestroyFramebuffer(context->device->device, frameBuffer, nullptr);
        context = nullptr;
    }
}