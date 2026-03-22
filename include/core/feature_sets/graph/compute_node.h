#pragma once
#include <render_node_with_bindings.h>
#include <compute.h>
#include <handles.h>
#include <glm/glm.hpp>

template<typename... Bindings>
struct ComputeNode: RenderNodeWithBindings<Bindings...> {

    ComputeNode(RenderContext& c, Ref<ComputePipeline> pipeline, QueueType queue=QueueType::Compute): 
        RenderNodeWithBindings<Bindings...>(c), pipeline(pipeline), queue(queue), dispatchSize(1, 1, 1)
    {
        assert(queue == QueueType::Compute || queue == QueueType::Graphics);
    }

    void SetGroups(uint32_t x, uint32_t y = 1, uint32_t z = 1) {
        SetGroups(glm::uvec3(x, y, z));
    }

    void SetGroups(glm::uvec3 xyz) {
        assert(xyz.x >= 1);
        assert(xyz.y >= 1);
        assert(xyz.z >= 1);
        dispatchSize = xyz;
    }

    QueueType getTargetQueue() {
        return QueueType::Compute;
    }

    void Record(ExecutionContext executionContext) {
        assert(executionContext.commandBuffer != nullptr);

        ComputeCommandBuffer& cmd = *static_cast<ComputeCommandBuffer*>(executionContext.commandBuffer);

        cmd.BeginComputePass(pipeline);

        auto _ = Helpers::allocator(&context).BeginContext();

        MemChunk<ShaderInputInstance> inputs = getShaderInputs();

        cmd.BindShaderInput(inputs.size, inputs.data);

        vkCmdDispatch(cmd.buffer, dispatchSize.x, dispatchSize.y, dispatchSize.z);
        
        cmd.EndPass();
    }

private:
    glm::uvec3 dispatchSize;
    Ref<ComputePipeline> pipeline;
    QueueType queue;
};