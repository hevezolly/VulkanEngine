#pragma once
#include <feature_set.h>
#include <device.h>

struct CommandPools: FeatureSet {
    using FeatureSet::FeatureSet;

    virtual void Init();
    virtual void Destroy();

    VkCommandPool graphicsCommandPool;
};