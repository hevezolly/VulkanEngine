#include <buffer.h>
#include <cassert>

const BufferPreset BufferPreset::VERTEX = {
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    QueueType::Graphics | QueueType::Transfer
};

const BufferPreset BufferPreset::INDEX = {
    VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    QueueType::Graphics | QueueType::Transfer
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
    convert(QueueType::Graphics)
};

Buffer::Buffer(VkDevice device, VkBuffer buffer, Memory&& mem):
    memory(std::move(mem)),
    vkBuffer(buffer),
    vkDevice(device)
{
    size_bytes = memory.size_bytes;
    vkBindBufferMemory(vkDevice, vkBuffer, memory.vkMemory, memory.offset);
}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
    if (this == &other)
        return *this;

    vkBuffer = other.vkBuffer;
    vkDevice = other.vkDevice;
    size_bytes = other.size_bytes;
    memory = std::move(other.memory);
    other.vkBuffer = VK_NULL_HANDLE;
    other.vkDevice = VK_NULL_HANDLE;

    return *this;
}

Buffer::Buffer(Buffer&& other) noexcept {
    *this = std::move(other);
}

Buffer::~Buffer() {
    if (vkDevice != VK_NULL_HANDLE) {
        vkDestroyBuffer(vkDevice, vkBuffer, nullptr);
        vkDevice = VK_NULL_HANDLE;
    }
}