#include <resource_id.h>

static const uint32_t TYPE_BITS = 1;
static const uint32_t CHILDREN_BITS = 8;
static const uint32_t ID_BITS = 32 - TYPE_BITS - CHILDREN_BITS;

static const uint32_t CHILDREN_MASK = ((0xffffffff >> TYPE_BITS) & (0xffffffff << ID_BITS));

std::size_t std::hash<ResourceId>::operator()(const ResourceId& i) const {
    return std::hash<uint32_t>{}(i.id);
}

bool ResourceId::operator==(ResourceId other) {
        return id == other.id;
    }

bool ResourceId::operator!=(ResourceId other) {
    return !(id == other.id);
}

ResourceType ResourceId::type() {
    return static_cast<ResourceType>((id >> (ID_BITS + CHILDREN_BITS)) & TYPE_BITS);
}

bool ResourceId::isChild() {
    return (((id & CHILDREN_MASK) >> ID_BITS)) > 0; 
}

ResourceId ResourceId::parent() {
    return {id & ~CHILDREN_MASK};
}