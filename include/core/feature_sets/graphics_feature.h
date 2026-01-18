#pragma once
#include <feature_set.h>
#include <common.h>
#include <vector>
#include <shader_source.h>
#include <optional>
#include <handles.h>
#include <frame_buffer.h>
#include <descriptor_pool.h>
#include <allocator_feature.h>
#include <shader_module.h>

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
    
    std::vector<ShaderModule> shaderModules;
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

    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> colorAttachmentRef;
    std::optional<VkPipelineDepthStencilStateCreateInfo> dsState;
    std::optional<VkAttachmentReference> dsRef;
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

    template<typename T>
    GraphicsPipelineBuilder& SetAttachments(const typename T::Formats& formats) {
        attachments.clear();
        T::GetAttachmentDescriptions(attachments, formats);
        colorAttachmentRef.clear();
        T::GetColorAttachmentReferences(colorAttachmentRef);

        if (T::size_depth_stencil() > 0) {
            dsRef = T::GetDepthStencilAttachmentReference();
            if (!dsState.has_value()) {
                dsState = VkPipelineDepthStencilStateCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
                dsState.value().depthTestEnable = VK_TRUE;
                dsState.value().depthWriteEnable = VK_TRUE;
                dsState.value().depthCompareOp = VK_COMPARE_OP_LESS;
                dsState.value().depthBoundsTestEnable = VK_FALSE;
                dsState.value().minDepthBounds = 0.0f;
                dsState.value().maxDepthBounds = 1.0f; 
                dsState.value().stencilTestEnable = VK_FALSE;
                dsState.value().front = {}; 
                dsState.value().back = {};
            }
        }

        return *this;
    }

    Ref<GraphicsPipeline> Build();

    friend struct GraphicsFeature;

private:
    Descriptors& getDescriptors();
    GraphicsPipelineBuilder(RenderContext& context);

    RenderContext* context;
};

struct API GraphicsFeature: FeatureSet,
    CanHandle<CollectRequiredQueueTypesMsg>,
    CanHandle<BeginFrameMsg>,
    CanHandle<DestroyMsg>
{

    using FeatureSet::FeatureSet;

    GraphicsPipelineBuilder NewGraphicsPipeline();

    template<typename T>
    const FrameBuffer& CreateFrameBuffer(T& args, VkRenderPass renderPass) {
        Allocator& a = getAllocator();
        MemChunk<VkImageView> views = a.BumpAllocate<VkImageView>(T::size());
        MemChunk<VkClearValue> clearVals = a.BumpAllocate<VkClearValue>(T::size());
        args.FillAttachments(views.data, clearVals.data);
        FrameBuffer result(&context, renderPass, views.data, clearVals.data, views.size, args.width(), args.height());
        a.Free(clearVals);
        a.Free(views);
        
        if (_frameBuffers.size() <= currentFrame) {
            _frameBuffers.resize(currentFrame + 1);
        }

        _frameBuffers[currentFrame].push_back(std::make_unique<FrameBuffer>(std::move(result)));
        
        return *_frameBuffers[currentFrame].back();
    }

    virtual void OnMessage(BeginFrameMsg*);

    virtual void OnMessage(CollectRequiredQueueTypesMsg*);
    
    virtual void OnMessage(DestroyMsg*);

private:
    Allocator& getAllocator();

    uint32_t currentFrame;
    std::vector<std::vector<std::unique_ptr<FrameBuffer>>> _frameBuffers; 
};

