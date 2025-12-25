#include <resource_memory.h>

MemoryChunkData::MemoryChunkData(VkDeviceMemory mem, VkDevice d):
    mappingStart(UINT32_MAX),
    mappingEnd(0),
    mappedptr(nullptr),
    mappings(0),
    vkMemory(mem),
    device(d)
{}

void MemoryChunkData::Map(uint32_t offset, uint32_t size) {
    if (mappedptr == nullptr || offset < mappingStart || size + offset > mappingEnd) {
        if (mappedptr != nullptr) {
            vkUnmapMemory(device, vkMemory);
        }

        vkMapMemory(device, vkMemory, offset, size, 0, &mappedptr);
        mappingStart = offset;
        mappingEnd = size + offset;
    }
    mappings++;
}

void MemoryChunkData::Unmap() {
    mappings--;

    if (mappings == 0) {
        vkUnmapMemory(device, vkMemory);
        mappedptr = nullptr;
        mappingStart = UINT32_MAX;
        mappingEnd = 0;
    }
}

Memory::Memory(VkDeviceMemory mem, VkDevice device, uint32_t size): 
    vkMemory(mem), 
    vkDevice(device),
    size_bytes(size),
    offset(0)
{
    mapData = new MemoryChunkData(mem, device);
}

Memory::Memory(Memory* parent, uint32_t offset, uint32_t size) {
    assert(parent->size_bytes > offset + size);

    vkMemory = vkMemory;
    mapData = parent->mapData;
    this->offset = parent->offset + offset;
    size_bytes = size;
    mapData->childCount++;
}

Memory::Memory():
    vkMemory(VK_NULL_HANDLE),
    vkDevice(VK_NULL_HANDLE),
    size_bytes(0),
    mapData(nullptr),
    offset(0),
    map(std::nullopt)
{}

Memory& Memory::operator=(Memory&& other) noexcept {
    if (&other == this)
        return *this;

    vkMemory = other.vkMemory;
    vkDevice = other.vkDevice;
    offset = other.offset;
    size_bytes = other.size_bytes;
    map = std::move(other.map);
    mapData = other.mapData;

    other.map = std::nullopt;
    other.size_bytes = 0;
    other.mapData = nullptr;
    other.vkMemory = VK_NULL_HANDLE;
    other.offset = 0;

    return *this;
}

Memory::Memory(Memory&& other) noexcept {
    *this = std::move(other);
}

Memory::~Memory() {
    Unmap();
    if (vkDevice != VK_NULL_HANDLE) {
        assert(mapData->childCount == 0);
        assert(mapData->mappings == 0);
        delete mapData;
        vkFreeMemory(vkDevice, vkMemory, nullptr);
    } else if (mapData) {
        mapData->childCount--;
    }
}

MemoryMapToken& Memory::PersistentMap() {
    if (!map.has_value())
        map.emplace(mapData, offset, size_bytes);

    return map.value();
}

void Memory::Unmap() {
    if (map.has_value())
        map.reset();
}

MemoryMapToken::MemoryMapToken(MemoryChunkData* mapData, uint32_t o, uint32_t size):
    mem(mapData),
    offset(o)
{
    mem->Map(o, size);
}

void* MemoryMapToken::data() {
    if (mem) {
        assert(offset >= mem->mappingStart);
        return static_cast<char*>(mem->mappedptr) + (offset - mem->mappingStart);
    }
    return nullptr;
}
 
MemoryMapToken::~MemoryMapToken() {
    if (mem) {
        mem->Unmap();
        mem = nullptr;
    }
}

MemoryMapToken& MemoryMapToken::operator=(MemoryMapToken&& other) noexcept {
    if (this == &other)
        return *this;

    mem = other.mem;
    offset = other.offset;
    other.mem = nullptr;

    return *this;
}

MemoryMapToken::MemoryMapToken(MemoryMapToken&& other) noexcept {
    *this = std::move(other);
}

MemoryMapToken Memory::Map(uint32_t size, uint32_t o) {
    assert(o + size <= size_bytes);
    return MemoryMapToken(mapData, offset + o, size);
}

MemoryMapToken Memory::Map() {
    return MemoryMapToken(mapData, offset, size_bytes);
}