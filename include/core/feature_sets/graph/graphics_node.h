#pragma once
#include <render_node.h>
#include <resource_storage.h>
#include <buffer.h>
#include <render_context.h>
#include <graphics_feature.h>
#include <allocator_feature.h>

template<typename Bindings, typename Attachments>
struct GraphicsNode: RenderNode {

    GraphicsNode(
        RenderContext* c, 
        Ref<GraphicsPipeline> p,
        const Bindings& bindings, 
        const Attachments& attachments):
        _attachments(attachments), context(c), pipeline(p), _bindings(bindings){}

    GraphicsNode(
        RenderContext* c, 
        Ref<GraphicsPipeline> p,
        const Attachments& attachments): 
        _attachments(attachments), context(c), pipeline(p), _bindings(std::nullopt){}

    template<typename Vertex>
    void SetVertexBuffer(ResourceRef<Buffer> vertex) {
        _vertexSize = sizeof(Vertex);
        _vertexBuffer = vertex;
    }

    void SetIndexBuffer(ResourceRef<Buffer> index, VkIndexType indexType) {
        _indexBuffer = index;
        _indexType = indexType;
    }

    void SetDrawCount(uint32_t count) {
        _drawCount = count;
    }

    virtual QueueType getTargetQueue() {
        return QueueType::Graphics;
    }

    virtual MemChunk<NodeDependency> getInputDependencies() {
        uint32_t size = 0;
        if (_bindings.has_value())
            size += Bindings::size_inputs();

        if (size == 0)
            return MemChunk<NodeDependency>::Null();
        
        MemChunk<NodeDependency> result = context->Get<Allocator>().BumpAllocate<NodeDependency>(size);
        _bindings.value().write_inputs(result.data);
        return result;
    }

    virtual MemChunk<NodeDependency> getOutputDependencies() {
        uint32_t size = Attachments::size();
        if (_bindings.has_value())
            size += Bindings::size_outputs();

        if (size == 0)
            return MemChunk<NodeDependency>::Null();
        
        MemChunk<NodeDependency> result = context->Get<Allocator>().BumpAllocate<NodeDependency>(size);
        _attachments.write_outputs(result.data);
        
        if (_bindings.has_value() && Bindings::size_outputs() > 0)
            _bindings.value().write_outputs(result.data + Attachments::size());
        return result;
    }

    virtual void Record(TransferCommandBuffer& commandBuffer) {
        GraphicsCommandBuffer& cmd = dynamic_cast<GraphicsCommandBuffer&>(commandBuffer);
        
        const FrameBuffer& frameBuffer = context->Get<GraphicsFeature>()
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

        if (_vertexBuffer.has_value()) {
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd.buffer, 0, 1, &_vertexBuffer.value()->vkBuffer, &offset);
        }

        if (_indexBuffer.has_value()) {
            vkCmdBindIndexBuffer(cmd.buffer, _indexBuffer.value()->vkBuffer, 0, _indexType);
        }

        if (_bindings.has_value()) {

            DescriptorSet& set = context->Get<Descriptors>().BorrowDescriptorSet(_bindings.value());

            vkCmdBindDescriptorSets(cmd.buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 0, 1, &set.vkSet, 0, nullptr);
        }

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

            vkCmdDrawIndexed(cmd.buffer, drawCount, 1, 0, 0, 0);
        }
        else {
            uint32_t drawCount = 0;
            assert(_vertexBuffer.has_value() || _drawCount.has_value());
            if (_vertexBuffer.has_value())
                drawCount = _vertexBuffer.value()->size_bytes() / _vertexSize;

            drawCount = _drawCount.value_or(drawCount);
            vkCmdDraw(cmd.buffer, drawCount, 1, 0, 0);
        }


        cmd.EndRenderPass();
    }

private:
    RenderContext* context;
    Ref<GraphicsPipeline> pipeline;
    std::optional<uint32_t> _drawCount;
    uint32_t _vertexSize;
    VkIndexType _indexType;
    std::optional<ResourceRef<Buffer>> _vertexBuffer;
    std::optional<ResourceRef<Buffer>> _indexBuffer;
    std::optional<Bindings> _bindings;
    Attachments _attachments;
};