#pragma once
#include <common.h>
#include <resource_memory.h>

struct API BufferPreset {
    VkBufferUsageFlags usageFlags; 
    VkMemoryPropertyFlags memoryProperties;
    QueueTypes usedQueues;

    static const BufferPreset VERTEX;
    static const BufferPreset INDEX;
    static const BufferPreset STAGING;
    static const BufferPreset UNIFORM;
};

inline BufferPreset& operator |=(BufferPreset& lhs, BufferPreset rhs) {
    lhs.usageFlags |= rhs.usageFlags;
    lhs.memoryProperties |= rhs.memoryProperties;
    return lhs;
}

inline BufferPreset& operator |=(BufferPreset& lhs, VkBufferUsageFlagBits usage) {
    lhs.usageFlags |= usage;
    return lhs;
}

inline BufferPreset& operator |=(BufferPreset& lhs, VkMemoryPropertyFlagBits mem) {
    lhs.memoryProperties |= mem;
    return lhs;
}

inline BufferPreset operator |(BufferPreset lhs, BufferPreset rhs) {
    lhs |= rhs;
    return lhs;
}

inline BufferPreset operator |(BufferPreset lhs, VkBufferUsageFlagBits rhs) {
    lhs |= rhs;
    return lhs;
}

inline BufferPreset operator |(BufferPreset lhs, VkMemoryPropertyFlagBits rhs) {
    lhs |= rhs;
    return lhs;
}

struct API Buffer {
    Memory memory;
    VkBuffer vkBuffer;
    uint32_t size_bytes;

    Buffer(VkDevice device, VkBuffer buffer, Memory&& memory);

    template<typename T>
    uint32_t count() {
        return size_bytes / sizeof(T);
    }

    RULE_5(Buffer)

private:
    VkDevice vkDevice;
};