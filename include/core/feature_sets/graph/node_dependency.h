#pragma once
#include <common.h>
#include <resource_id.h>

struct API NodeDependency {
    ResourceId resource;
    VkPipelineStageFlagBits stage;
    VkImageLayout layout;
};