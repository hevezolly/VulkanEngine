#pragma once

#include <feature_set.h>

struct DescriptorPool 
{
    VkDescriptorPool vkPool;
    DescriptorPool(RenderContext*, std::initializer_list<VkDescriptorPoolSize> sizes, bool createFree=false);
    
    RULE_5(DescriptorPool)
private:
    RenderContext* context;
};