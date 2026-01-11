#pragma once
#include <common.h>
#include <node_dependency.h>
#include <allocator_feature.h>
#include <command_pool.h>

struct API RenderNode {
    
    virtual QueueType getTargetQueue() = 0;
    virtual MemChunk<NodeDependency> getInputDependencies() = 0;
    virtual MemChunk<NodeDependency> getOutputDependencies() = 0;

    virtual void Record(TransferCommandBuffer& commandBuffer) = 0;
};