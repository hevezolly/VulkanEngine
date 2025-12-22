#pragma once
#include <common.h>

struct API MemoryMapToken {
    void* data;

    MemoryMapToken(VkDevice, VkDeviceMemory, uint32_t offset, uint32_t size);

    RULE_5(MemoryMapToken)

private: 
    VkDevice device;
    VkDeviceMemory memory;
};

struct API Memory {
    uint32_t offset;
    VkDeviceMemory vkMemory;

    Memory(VkDeviceMemory memory, VkDevice device);
    MemoryMapToken Map(uint32_t size, uint32_t offset=0);
    Memory();

    RULE_5(Memory)
    
private:
    VkDevice vkDevice;
};