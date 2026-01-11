#pragma once
#include <common.h>
#include <resource_id.h>

struct API NodeDependency {
    ResourceId type;
    VkPipelineStageFlagBits stage;
    VkImageLayout layout;
};