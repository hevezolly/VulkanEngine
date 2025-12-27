#pragma once
#include <common.h>

enum API Stage {
    Fragment = VK_SHADER_STAGE_FRAGMENT_BIT,
    Vertex = VK_SHADER_STAGE_VERTEX_BIT,
    Compute = VK_SHADER_STAGE_COMPUTE_BIT
};

inline Stage operator | (Stage lhs, Stage rhs) {
    return static_cast<Stage>(static_cast<VkShaderStageFlagBits>(lhs) | static_cast<VkShaderStageFlagBits>(rhs));
}

inline Stage operator & (Stage lhs, Stage rhs) {
    return static_cast<Stage>(static_cast<VkShaderStageFlagBits>(lhs) & static_cast<VkShaderStageFlagBits>(rhs));
}

inline VkShaderStageFlagBits ToVkShaderStage(Stage stage) {
    return static_cast<VkShaderStageFlagBits>(stage);
}