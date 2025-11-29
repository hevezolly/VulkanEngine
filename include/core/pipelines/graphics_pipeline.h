#pragma once
#include <common.h>
#include <vector>
#include <render_context.h>
#include <shader_source.h>
#include <volk.h>
#include <optional>

struct BlendMethod {
    VkBlendFactor src;
    VkBlendFactor dst;
    VkBlendOp op;
};

struct GraphicsPipeline {
    VkPipelineLayout layout;
    VkRenderPass renderPass;

    RULE_5(GraphicsPipeline)

    friend struct GraphicsPipelineBuilder;
private: 
    RenderContext* context;
};

struct API GraphicsPipelineBuilder {
    
    std::vector<VkShaderModule> shaderModules;
    std::vector<VkPipelineShaderStageCreateInfo> stages;
    std::vector<VkDynamicState> dynamicStates;
    std::optional<VkViewport> viewport;
    std::optional<VkRect2D> scissor;
    VkPipelineInputAssemblyStateCreateInfo inputAssembly;
    VkPipelineRasterizationStateCreateInfo rasterization;
    VkPipelineMultisampleStateCreateInfo multisampling;
    VkPipelineColorBlendAttachmentState blending;
    VkPipelineColorBlendStateCreateInfo blendingCreateInfo;

    //TODO: rework
    VkPipelineLayoutCreateInfo pipelineLayout;
    VkAttachmentDescription colorAttachment;
    VkAttachmentReference colorAttachmentRef;
    VkSubpassDescription subpass;

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

    GraphicsPipelineBuilder& SetDepthClampEnable(bool enable);

    GraphicsPipelineBuilder& SetPolygoneMode(VkPolygonMode mode);

    GraphicsPipelineBuilder& SetColorBlending(BlendMethod& method);
    GraphicsPipelineBuilder& SetAlphaBlending(BlendMethod& method);

    GraphicsPipeline* Build();

private:
    RenderContext* context;
};