#pragma once
#include <feature_set.h>
#include <common.h>
#include <vector>
#include <shader_source.h>
#include <optional>
#include <handles.h>

struct API BlendMethod {
    VkBlendFactor src;
    VkBlendFactor dst;
    VkBlendOp op;
};

struct API GraphicsPipeline {
    VkPipelineLayout layout;
    VkRenderPass renderPass;
    VkPipeline pipeline;

    GraphicsPipeline();

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
    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
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

    Ref<GraphicsPipeline> Build();

    RULE_5(GraphicsPipelineBuilder)

    friend struct GraphicsFeature;

private:
    GraphicsPipelineBuilder(RenderContext& context);

    RenderContext* context;
};

struct GraphicsFeature: FeatureSet {
    using FeatureSet::FeatureSet;

    GraphicsPipelineBuilder GraphicsPipeline();
};