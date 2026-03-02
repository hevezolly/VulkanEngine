#pragma once
#include <common.h>

struct ResourceState {
    VkAccessFlags2 currentAccess;
    VkPipelineStageFlags2 accessStage;
    VkImageLayout currentLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;

    ResourceState(
        VkAccessFlags2 access, 
        VkPipelineStageFlags2 stage, 
        VkImageLayout layout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED): 
        currentAccess(access),
        accessStage(stage),
        currentLayout(layout) {}

    ResourceState(): ResourceState(VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT) {}
};

namespace ResourceIdLimits {

    inline constexpr uint32_t SIZE = 64;
    inline constexpr uint32_t TYPE_BITS = 2;
    inline constexpr uint32_t GENERATION_BITS = 30;
    inline constexpr uint32_t ID_BITS = SIZE - TYPE_BITS - GENERATION_BITS;

    inline constexpr uint64_t GENERATION_MASK = 
        ((UINT64_MAX >> TYPE_BITS) & 
        (UINT64_MAX << ID_BITS));

    inline constexpr uint64_t PARENT_IDS_COUNT = (uint64_t(1) << ID_BITS);
    inline constexpr uint64_t RESOURCE_TYPES_COUNT = (uint64_t(1) << TYPE_BITS);
    inline constexpr uint64_t GENERATION_COUNT = (uint64_t(1) << GENERATION_BITS);
}

enum struct ResourceType: uint8_t {
    Other = 0,
    Image = 1,
    Buffer = 2,
    Sampler = 3
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
    
