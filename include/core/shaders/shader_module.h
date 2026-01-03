#pragma once
#include <common.h>

struct ShaderModule {
    VkShaderModule vkModule;

    ShaderModule(VkShaderModule module, VkDevice vkDevice): 
        vkModule(module), device(vkDevice) {}

    RULE_5(ShaderModule)

private:
    VkDevice device;
};