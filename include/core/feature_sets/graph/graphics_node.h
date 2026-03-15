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


template<typename Attachments, typename... Bindings>
struct GraphicsNode: RenderNodeWithBindings<Bindings...> {

    void SetAttachments(const Attachments& a) {
        _attachments = a;
    }

    GraphicsNode(
        RenderContext& c, 
        Ref<GraphicsPipeline> p): RenderNodeWithBindings<Bindings...>(c), 
        pipeline(p), _instanceCount(1){} 

    void AddVertexBuffer(ResourceRef<Buffer> vertex, VkDeviceSize offset = 0) {
        _vertexBuffers.push_back(vertex);
        _offsets.push_back(offset);
    }

    void SetIndexBuffer(ResourceRef<Buffer> index, VkIndexType indexType) {
        _indexBuffer = index;
        _indexType = indexType;
    }

    void SetInstanceCount(uint32_t count) {
        assert(count > 0);
        _instanceCount = count;
    }

    void SetDrawCount(uint32_t count) {
        _drawCount = count;
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

        assert(commandBuffer.commandBuffer != nullptr);

        GraphicsCommandBuffer* c = dynamic_cast<GraphicsCommandBuffer*>(commandBuffer.commandBuffer);

        assert(c != nullptr);

        GraphicsCommandBuffer& cmd = dynamic_cast<GraphicsCommandBuffer&>(*commandBuffer.commandBuffer);
        
        const FrameBuffer& frameBuffer = context.Get<GraphicsFeature>()
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

        auto _ = context.Get<Allocator>().BeginContext();


        if (_vertexBuffers.size() > 0) {

            MemChunk<VkBuffer> vertexBuffers = context.Get<Allocator>().BumpAllocate<VkBuffer>(_vertexBuffers.size());

            for (int i = 0; i < vertexBuffers.size; i++) {
                vertexBuffers[i] = _vertexBuffers[i]->vkBuffer;
            }

            vkCmdBindVertexBuffers(cmd.buffer, 0, _vertexBuffers.size(), vertexBuffers.data, _offsets.data());
        }

        if (_indexBuffer.has_value()) {
            vkCmdBindIndexBuffer(cmd.buffer, _indexBuffer.value()->vkBuffer, 0, _indexType);
        }

        MemChunk<ShaderInputInstance> inputBuffer = getShaderInputs();

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

            uint32_t drawCount = _drawCount.value_or(_indexBuffer.value()->size_bytes() / indexSize);

            vkCmdDrawIndexed(cmd.buffer, drawCount, _instanceCount, 0, 0, 0);
        }
        else {
            uint32_t drawCount = 0;
            assert(_drawCount.has_value());

            drawCount = _drawCount.value();
            vkCmdDraw(cmd.buffer, drawCount, _instanceCount, 0, 0);
        }


        cmd.EndPass();
    }

private:

    uint32_t _instanceCount;
    Ref<GraphicsPipeline> pipeline;
    std::optional<uint32_t> _drawCount;
    VkIndexType _indexType;
    std::vector<ResourceRef<Buffer>> _vertexBuffers;
    std::vector<VkDeviceSize> _offsets;
    std::optional<ResourceRef<Buffer>> _indexBuffer;
    Attachments _attachments;
};