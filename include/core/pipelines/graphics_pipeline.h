#pragma once
#include <common.h>
#include <vector>
#include <render_context.h>
#include <shader_source.h>
#include <volk.h>

struct API GraphicsPipelineBuilder {
    
    std::vector<VkShaderModule> shaderModules;
    std::vector<VkPipelineShaderStageCreateInfo> stages;

    GraphicsPipelineBuilder(RenderContext& context);

    GraphicsPipelineBuilder& AddShaderStage(ShaderStage stage, ShaderBinary& binary);


private:
    RenderContext* context;
};