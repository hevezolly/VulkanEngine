#pragma once
#include <common.h>

enum struct ResourceType: uint8_t {
    Image = 0,
    Buffer = 1
};

struct API ResourceId {
    uint32_t id;

    ResourceType type();

    bool isChild();
    ResourceId parent();

    bool operator==(ResourceId other);
    bool operator!=(ResourceId other);
};

namespace std {
    template <>
    struct API hash<ResourceId> {
        size_t operator()(const ResourceId& p) const noexcept;
    };
}
    
