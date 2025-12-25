#pragma once
#include <feature_set.h>
#include <common.h>
#include <vector>
#include <shader_source.h>
#include <optional>
#include <handles.h>
#include <frame_buffer.h>
#include <descriptor_pool.h>

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

    Ref<FrameBuffer> CreateFrameBuffer(ImageView* image) {
        return context->New<FrameBuffer>(context, image, renderPass);
    }

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
    
    std::vector<VkVertexInputBindingDescription> vertexDescriptions;
    std::vector<VkVertexInputAttributeDescription> vertexAttributes;
    std::vector<VkDescriptorSetLayout> descriptorLayouts;

    //TODO: rework
    VkAttachmentDescription colorAttachment;
    VkAttachmentReference colorAttachmentRef;
    VkSubpassDescription subpass;

    GraphicsPipelineBuilder& AddShaderStage(Stage stage, ShaderBinary& binary);

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

    template<typename T>
    GraphicsPipelineBuilder& SetVertex() {
        vertexDescriptions.resize(1);
        vertexDescriptions[0] = T::GetBindingDescription();
        vertexAttributes.clear();
        T::CollectAttributeDescription(vertexAttributes);
        return *this;
    }

    template<typename T>
    GraphicsPipelineBuilder& AddLayout() {
        descriptorLayouts.push_back(getDescriptors().GetLayout<T>());
        return *this;
    }

    Ref<GraphicsPipeline> Build();

    RULE_5(GraphicsPipelineBuilder)

    friend struct GraphicsFeature;

private:
    Descriptors& getDescriptors();
    GraphicsPipelineBuilder(RenderContext& context);

    RenderContext* context;
};

struct API GraphicsFeature: FeatureSet,
    CanHandle<CollectRequiredQueueTypesMsg>
{

    using FeatureSet::FeatureSet;

    GraphicsPipelineBuilder GraphicsPipeline();

    virtual void OnMessage(CollectRequiredQueueTypesMsg*);
};