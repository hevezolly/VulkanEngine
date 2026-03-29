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
#include <pipeline_builder.h>
#include <framed_storage.h>

struct API BlendMethod {
    VkBlendFactor src;
    VkBlendFactor dst;
    VkBlendOp op;
};

enum struct VertexInputRate: uint32_t {
    Vertex = VK_VERTEX_INPUT_RATE_VERTEX,
    Instance = VK_VERTEX_INPUT_RATE_INSTANCE
};

struct API GraphicsPipeline: Pipeline {
    VkRenderPass renderPass;

    GraphicsPipeline(VkDevice device, VkPipeline pipeline, VkPipelineLayout layout, VkRenderPass rp):
        Pipeline(device, pipeline, layout), renderPass(rp){}

    RULE_5(GraphicsPipeline)
};

struct API GraphicsPipelineBuilder: PipelineBuilder<GraphicsPipelineBuilder> {
    
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

    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> colorAttachmentRef;
    std::optional<VkPipelineDepthStencilStateCreateInfo> dsState;
    std::optional<VkAttachmentReference> dsRef;
    VkSubpassDescription subpass;
    uint32_t location;

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
    GraphicsPipelineBuilder& AddVertex(VertexInputRate inputRate = VertexInputRate::Vertex) {

        uint32_t binding = vertexDescriptions.size();

        vertexDescriptions.push_back(VkVertexInputBindingDescription {
            binding, sizeof(T), static_cast<VkVertexInputRate>(inputRate) 
        });

        T::CollectAttributeDescription(vertexAttributes, binding, location);
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

protected:
    GraphicsPipelineBuilder& getSelf() {
        return *this;
    }

private:
    GraphicsPipelineBuilder(RenderContext& context);
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
        Allocator& a = Helpers::allocator(&context);
        auto _ = a.BeginContext();
        MemChunk<VkImageView> views = a.BumpAllocate<VkImageView>(T::size());
        MemChunk<VkClearValue> clearVals = a.BumpAllocate<VkClearValue>(T::size());
        args.FillAttachments(views.data, clearVals.data);
        FrameBuffer result(&context, renderPass, views.data, clearVals.data, views.size, args.width(), args.height());

        _frameBuffers->push_back(std::make_unique<FrameBuffer>(std::move(result)));
        
        return *(_frameBuffers->back());
    }

    virtual void OnMessage(BeginFrameMsg*);

    virtual void OnMessage(CollectRequiredQueueTypesMsg*);
    
    virtual void OnMessage(DestroyMsg*);

private:

    FramedStorage<std::vector<std::unique_ptr<FrameBuffer>>> _frameBuffers;
};

