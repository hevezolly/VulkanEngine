#pragma once
#include <common.h>
#include <vector>
#include <render_context.h>
#include <shader_source.h>
#include <volk.h>
#include <optional>

struct API GraphicsPipelineBuilder {
    
    std::vector<VkShaderModule> shaderModules;
    std::vector<VkPipelineShaderStageCreateInfo> stages;
    std::vector<VkDynamicState> dynamicStates;
    std::optional<VkViewport> viewport;
    std::optional<VkRect2D> scissor;
    VkPipelineInputAssemblyStateCreateInfo inputAssembly;
    VkPipelineRasterizationStateCreateInfo rasterization;

    GraphicsPipelineBuilder(RenderContext& context);

    GraphicsPipelineBuilder& AddShaderStage(ShaderStage stage, ShaderBinary& binary);

    GraphicsPipelineBuilder& AddDynamicState(VkDynamicState state);

    GraphicsPipelineBuilder& SetViewport(VkViewport viewport);
    
    GraphicsPipelineBuilder& SetScissor(VkRect2D scissor);

    GraphicsPipelineBuilder& SetTopology(
        VkPrimitiveTopology topology, bool restartEnabled=false);

    GraphicsPipelineBuilder& SetCullMode(
        VkCullModeFlagBits mode, VkFrontFace frontFace = VK_FRONT_FACE_CLOCKWISE
    );

private:
    RenderContext* context;
};