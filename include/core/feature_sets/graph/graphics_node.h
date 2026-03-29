#pragma once
#include <render_node.h>
#include <resource_storage.h>
#include <buffer.h>
#include <render_context.h>
#include <graphics_feature.h>
#include <allocator_feature.h>
#include <tuple>
#include <utility>
#include <render_node_with_bindings.h>

struct DrawParameters {
    uint32_t drawCount;
    uint32_t instanceCount;
    uint32_t instanceOffset;    
    
    uint32_t offset;
    int32_t vertexOffset;
    ShaderDynamicState dynamicState;
    
    DrawParameters(uint32_t count, uint32_t offset, ShaderDynamicState dynamicState={}): 
        drawCount(count), offset(offset), vertexOffset(0), instanceCount(1), instanceOffset(0), dynamicState(dynamicState) {} 

    DrawParameters(uint32_t count, uint32_t offset, uint32_t instanceCount, uint32_t instanceOffset=0, int32_t vertexOffset=0, ShaderDynamicState dynamicState={}): 
        drawCount(count), offset(offset), vertexOffset(vertexOffset), instanceCount(instanceCount), instanceOffset(instanceOffset), dynamicState(dynamicState) {} 
};


template<typename Attachments, typename... Bindings>
struct GraphicsNode: RenderNodeWithBindings<Bindings...> {

    void SetAttachments(const Attachments& a) {
        _attachments = a;
    }

    GraphicsNode(
        RenderContext& c, 
        Ref<GraphicsPipeline> p): RenderNodeWithBindings<Bindings...>(c), 
        pipeline(p){} 

    void AddVertexBuffer(ResourceRef<Buffer> vertex, VkDeviceSize offset = 0) {
        _vertexBuffers.push_back(vertex);
        _offsets.push_back(offset);
    }

    void SetIndexBuffer(ResourceRef<Buffer> index, VkIndexType indexType) {
        _indexBuffer = index;
        _indexType = indexType;
    }

    void AddDrawParameters(DrawParameters p) {
        _drawParameters.push_back(p);
    }   

    virtual QueueType getTargetQueue() {
        return QueueType::Graphics;
    }

    virtual uint32_t getInputDependenciesCount() {
        uint32_t size = _vertexBuffers.size();
        if (_indexBuffer.has_value())
            size++;

        size += RenderNodeWithBindings<Bindings...>::getInputDependenciesCount();
        return size;
    }

    virtual constexpr uint32_t getOutputDependenciesCount() {
        uint32_t size = Attachments::size();

        size += RenderNodeWithBindings<Bindings...>::getOutputDependenciesCount();
        return size;
    }

    virtual void getInputDependencies(NodeDependency* dependencies) {
        uint32_t written = 0;

        for (int i = 0; i < _vertexBuffers.size(); i++) {
            dependencies[written++] = NodeDependency {
                _vertexBuffers[i].id,
                ResourceState {
                    VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT,
                    VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT
                }
            };
        }

        if (_indexBuffer.has_value()) {
            dependencies[written++] = NodeDependency {
                _indexBuffer.value().id,
                ResourceState {
                    VK_ACCESS_2_INDEX_READ_BIT,
                    VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT
                }
            };
        }

        RenderNodeWithBindings<Bindings...>::getInputDependencies(dependencies + written);
    }

    virtual void getOutputDependencies(NodeDependency* dependencies) {
        _attachments.write_outputs(dependencies);

        RenderNodeWithBindings<Bindings...>::getOutputDependencies(dependencies + Attachments::size());
    }

    virtual void Record(ExecutionContext commandBuffer) {

        ASSERT(commandBuffer.commandBuffer != nullptr);

        GraphicsCommandBuffer* c = dynamic_cast<GraphicsCommandBuffer*>(commandBuffer.commandBuffer);

        ASSERT(c != nullptr);

        GraphicsCommandBuffer& cmd = dynamic_cast<GraphicsCommandBuffer&>(*commandBuffer.commandBuffer);
        
        const FrameBuffer& frameBuffer = this->context.Get<GraphicsFeature>()
            .CreateFrameBuffer(_attachments, pipeline->renderPass);

        cmd.BeginRenderPass(pipeline, frameBuffer);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(frameBuffer.width);
        viewport.height = static_cast<float>(frameBuffer.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd.buffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = {frameBuffer.width, frameBuffer.height};
        vkCmdSetScissor(cmd.buffer, 0, 1, &scissor);

        auto _ = this->context.Get<Allocator>().BeginContext();


        if (_vertexBuffers.size() > 0) {

            MemChunk<VkBuffer> vertexBuffers = this->context.Get<Allocator>().BumpAllocate<VkBuffer>(_vertexBuffers.size());

            for (int i = 0; i < vertexBuffers.size; i++) {
                vertexBuffers[i] = _vertexBuffers[i]->vkBuffer;
            }

            vkCmdBindVertexBuffers(cmd.buffer, 0, _vertexBuffers.size(), vertexBuffers.data, _offsets.data());
        }

        if (_indexBuffer.has_value()) {
            vkCmdBindIndexBuffer(cmd.buffer, _indexBuffer.value()->vkBuffer, 0, _indexType);
        }

        MemChunk<ShaderInputInstance> inputBuffer = this->getShaderInputs();

        cmd.BindShaderInput(inputBuffer.size, inputBuffer.data);

        if (_indexBuffer.has_value()) {
            
            uint32_t indexSize = 0;
            switch(_indexType) {
                case VkIndexType::VK_INDEX_TYPE_UINT32:
                    indexSize = 4;
                    break;
                case VkIndexType::VK_INDEX_TYPE_UINT16:
                    indexSize = 2;
                    break;
                case VkIndexType::VK_INDEX_TYPE_UINT8:
                    indexSize = 1;
                    break;
                default:
                    throw std::runtime_error("unsupported index type");
            }

            if (_drawParameters.size() == 0) {
                vkCmdDrawIndexed(cmd.buffer, _indexBuffer.value()->size_bytes() / indexSize, 1, 0, 0, 0);
            }
            else {
                for (DrawParameters& params : _drawParameters) {

                    if (params.dynamicState.pDynamicOffsets != nullptr) {
                        cmd.UpdateDynamicState(params.dynamicState);
                    }

                    vkCmdDrawIndexed(cmd.buffer, params.drawCount, params.instanceCount, params.offset, params.vertexOffset, params.instanceOffset);
                }
            }

        }
        else {
            ASSERT_MSG(_drawParameters.size() > 0, "For vertex buffer only draw, parameters must be provided via AddDrawParameters");

            for (DrawParameters& params : _drawParameters) {

                if (params.dynamicState.pDynamicOffsets != nullptr) {
                    cmd.UpdateDynamicState(params.dynamicState);
                }

                vkCmdDraw(cmd.buffer, params.drawCount, params.instanceCount, params.offset, params.instanceOffset);
            }
        }


        cmd.EndPass();
    }

private:
    Ref<GraphicsPipeline> pipeline;
    std::vector<DrawParameters> _drawParameters;
    VkIndexType _indexType;
    std::vector<ResourceRef<Buffer>> _vertexBuffers;
    std::vector<VkDeviceSize> _offsets;
    std::optional<ResourceRef<Buffer>> _indexBuffer;
    Attachments _attachments;
};