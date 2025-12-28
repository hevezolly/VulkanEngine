#pragma once
#include <common.h>
#include <optional>

// not thread safe
struct MemoryChunkData {
    uint32_t mappingStart;
    uint32_t mappingEnd;
    void* mappedptr;
    uint32_t mappings;
    uint32_t childCount;
    VkDeviceMemory vkMemory;
    VkDevice device;

    MemoryChunkData(VkDeviceMemory memory, VkDevice device); 

    void Map(uint32_t offset, uint32_t size);
    void Flush(uint32_t offset, uint32_t size);
    void Unmap();
};

struct API MemoryMapToken {
    void* data();

    MemoryMapToken(MemoryChunkData* mem, uint32_t offset, uint32_t size);

    RULE_5(MemoryMapToken)

private: 
    uint32_t offset;
    MemoryChunkData* mem;
};

struct API Memory {
    uint32_t size_bytes;
    VkDeviceMemory vkMemory;
    uint32_t offset;
    
    Memory(VkDeviceMemory memory, VkDevice device, uint32_t size);
    MemoryMapToken Map(uint32_t size, uint32_t offset=0);
    MemoryMapToken Map();
    MemoryMapToken& PersistentMap();
    void Unmap();
    Memory();
    Memory(Memory* parent, uint32_t offset, uint32_t size);
    void Flush();

    RULE_5(Memory)
    
private:
    std::optional<MemoryMapToken> map; 
    MemoryChunkData* mapData;
    VkDevice vkDevice;
};