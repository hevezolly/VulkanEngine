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

Buffer::Buffer(VkDevice device, VkBuffer buffer, uint32_t size, Memory&& mem):
    memory(std::move(mem)),
    vkBuffer(buffer),
    vkDevice(device),
    size_bytes(size)
{
    vkBindBufferMemory(vkDevice, vkBuffer, memory.vkMemory, memory.offset);
}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
    if (this == &other)
        return *this;

    vkBuffer = other.vkBuffer;
    vkDevice = other.vkDevice;
    size_bytes = other.size_bytes;
    memory = std::move(other.memory);
    other.vkBuffer = nullptr;
    other.vkDevice = nullptr;

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

MemoryMapToken Buffer::Map() {
    return memory.Map(size_bytes);
}

MemoryMapToken Buffer::Map(uint32_t offset, uint32_t size) {
    assert(offset + size <= size_bytes);

    return memory.Map(size, offset);
}