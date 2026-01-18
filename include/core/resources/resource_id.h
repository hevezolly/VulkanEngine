#pragma once
#include <common.h>

namespace ResourceIdLimits {

    inline constexpr uint32_t SIZE = 64;
    inline constexpr uint32_t TYPE_BITS = 2;
    inline constexpr uint32_t GENERATION_BITS = 30;
    inline constexpr uint32_t ID_BITS = SIZE - TYPE_BITS - GENERATION_BITS;

    inline constexpr uint64_t GENERATION_MASK = 
        ((0xffffffff >> TYPE_BITS) & 
        (0xffffffff << ID_BITS));

    inline constexpr uint32_t PARENT_IDS_COUNT = (uint32_t(1) << ID_BITS);
    inline constexpr uint32_t RESOURCE_TYPES_COUNT = (uint32_t(1) << TYPE_BITS);
    inline constexpr uint32_t GENERATION_COUNT = (uint32_t(1) << GENERATION_BITS);
}

enum struct ResourceType: uint8_t {
    Image = 0,
    Buffer = 1,
    Sampler = 2,
    Other = 3
};

struct API ResourceId {
    uint64_t id;

    ResourceType type() const;
    uint32_t index() const;
    uint32_t generation() const;

    bool operator==(const ResourceId& other) const;
    bool operator!=(const ResourceId& other) const;

    static ResourceId Compose(ResourceType type, uint32_t id, uint32_t generation=0);

    ResourceId NextGeneration() const;
};


namespace std {
    template <>
    struct API hash<ResourceId> {
        size_t operator()(const ResourceId& p) const noexcept;
    };
}
    
