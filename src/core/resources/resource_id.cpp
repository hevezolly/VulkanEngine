#include <resource_id.h>

std::size_t std::hash<ResourceId>::operator()(const ResourceId& i) const noexcept {
    return std::hash<uint64_t>{}(i.id);
}

bool ResourceId::operator==(const ResourceId& other) const {
        return id == other.id;
    }

bool ResourceId::operator!=(const ResourceId& other) const {
    return !(*this == other);
}

ResourceType ResourceId::type() const {
    return static_cast<ResourceType>((id >> (
        ResourceIdLimits::ID_BITS + ResourceIdLimits::GENERATION_BITS)) &
        ResourceIdLimits::TYPE_BITS);
}

uint32_t ResourceId::index() const {
    return static_cast<uint32_t>((id & ~ResourceIdLimits::GENERATION_MASK) & 0xffffffff);
}

uint32_t ResourceId::generation() const {
    return static_cast<uint32_t>(((id & ResourceIdLimits::GENERATION_MASK) >> ResourceIdLimits::ID_BITS) & 0xffffffff);
}

ResourceId ResourceId::Compose(ResourceType type, uint32_t id, uint32_t generation) {
    uint64_t typeBits = static_cast<uint64_t>(type);
    assert(typeBits < ResourceIdLimits::RESOURCE_TYPES_COUNT);
    
    uint64_t result = typeBits << (ResourceIdLimits::SIZE - ResourceIdLimits::TYPE_BITS) | 
                      static_cast<uint64_t>(generation) << (ResourceIdLimits::ID_BITS) |
                      id;

    return {result};
}

ResourceId ResourceId::NextGeneration() const {
    assert(generation() < ResourceIdLimits::GENERATION_COUNT - 1);

    uint64_t result = id | (generation() + 1) << ResourceIdLimits::ID_BITS;
    
    return {result};
}