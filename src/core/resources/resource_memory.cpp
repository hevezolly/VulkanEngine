#include <resource_memory.h>

Memory::Memory(VkDeviceMemory mem, VkDevice device): 
    vkMemory(mem), 
    vkDevice(device),
    offset(0)
{}

Memory::Memory():
    vkMemory(VK_NULL_HANDLE),
    vkDevice(VK_NULL_HANDLE),
    offset(0)
{}

Memory& Memory::operator=(Memory&& other) noexcept {
    if (&other == this)
        return *this;

    vkMemory = other.vkMemory;
    vkDevice = other.vkDevice;
    offset = other.offset;
    other.vkMemory = VK_NULL_HANDLE;
    other.vkMemory = VK_NULL_HANDLE;
    other.offset = 0;

    return *this;
}

Memory::Memory(Memory&& other) noexcept {
    *this = std::move(other);
}

Memory::~Memory() {
    if (vkDevice != VK_NULL_HANDLE) {
        vkFreeMemory(vkDevice, vkMemory, nullptr);
    }
}

MemoryMapToken::MemoryMapToken(VkDevice d, VkDeviceMemory mem, uint32_t offset, uint32_t size):
    device(d),
    memory(mem)    
{
    vkMapMemory(d,mem, offset, size, 0, &data);
}
 
MemoryMapToken::~MemoryMapToken() {
    if (device != VK_NULL_HANDLE) {
        vkUnmapMemory(device, memory);
        device= VK_NULL_HANDLE;
    }
}

MemoryMapToken& MemoryMapToken::operator=(MemoryMapToken&& other) noexcept {
    if (this == &other)
        return *this;

    data = other.data;
    device = other.device;
    memory = other.memory;

    other.device = VK_NULL_HANDLE;
    other.memory = VK_NULL_HANDLE;
    other.data = nullptr;

    return *this;
}

MemoryMapToken::MemoryMapToken(MemoryMapToken&& other) noexcept {
    *this = std::move(other);
}

MemoryMapToken Memory::Map(uint32_t size, uint32_t o) {
    return MemoryMapToken(vkDevice, vkMemory, offset + o, size);
}