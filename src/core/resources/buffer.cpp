#include <buffer.h>
#include <cassert>
#include <render_context.h>
#include <resources.h>

const BufferPreset BufferPreset::VERTEX = {
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    QueueType::Graphics | QueueType::Compute | QueueType::Transfer
};

const BufferPreset BufferPreset::INDEX = {
    VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    QueueType::Graphics | QueueType::Compute | QueueType::Transfer
};

const BufferPreset BufferPreset::STAGING = {
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    convert(QueueType::Transfer)
};

const BufferPreset BufferPreset::UNIFORM = {
    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    QueueType::Graphics | QueueType::Compute
};

Buffer::Buffer(RenderContext& ctx, VkBuffer buffer, Memory&& mem):
    memory(std::move(mem)),
    vkBuffer(buffer),
    context(&ctx)
{
    vkBindBufferMemory(ctx.device(), vkBuffer, memory.vkMemory, memory.offset);
}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
    if (this == &other)
        return *this;

    vkBuffer = other.vkBuffer;
    context = other.context;
    memory = std::move(other.memory);
    other.vkBuffer = VK_NULL_HANDLE;
    other.context = nullptr;

    return *this;
}

Buffer::Buffer(Buffer&& other) noexcept {
    *this = std::move(other);
}

Buffer::~Buffer() {
    if (context != nullptr) {
        vkDestroyBuffer(context->device(), vkBuffer, nullptr);
        context = nullptr;
    }
}

uint32_t Buffer::size_bytes() {
    return memory.size_bytes;
}