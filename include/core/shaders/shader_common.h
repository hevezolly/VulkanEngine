#pragma once
#include <common.h>

enum struct API Stage {
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

inline VkPipelineStageFlags2 ToVkPipelineStage(Stage stage) {
    VkPipelineStageFlags2 result = 0;
    if ((stage & Stage::Fragment) == Stage::Fragment)
        result |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    if ((stage & Stage::Vertex) == Stage::Vertex)
        result |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
    if ((stage & Stage::Compute) == Stage::Compute)
        result |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;

    return result;
}

enum struct API LoadOp {
    Load = VK_ATTACHMENT_LOAD_OP_LOAD,
    Clear = VK_ATTACHMENT_LOAD_OP_CLEAR,
    Any = VK_ATTACHMENT_LOAD_OP_DONT_CARE
};

inline void ResolveAttachmentInitialLayout(VkImageLayout& layout, VkAttachmentLoadOp loadOp, VkAttachmentLoadOp stencilLoadOp) {
    if ((loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR || loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE) &&
        (stencilLoadOp == VK_ATTACHMENT_LOAD_OP_CLEAR || stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE))
        layout = VK_IMAGE_LAYOUT_UNDEFINED;
}