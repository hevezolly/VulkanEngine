#pragma once
#include <common.h>
#include <allocator_feature.h>
#include <command_pool.h>
#include <resource_id.h>

struct RenderContext;

struct API NodeDependency {
    ResourceId resource;
    ResourceState state;
};

struct API ExecutionContext {
    TransferCommandBuffer* commandBuffer;
    MemChunk<Semaphore> executionStart;
};

struct API RenderNode {
    
    RenderNode(RenderContext& ctx): context(ctx){}

    virtual bool requireBinarySemaphore() {return false;}
    virtual QueueType getTargetQueue() = 0;
    virtual uint32_t getInputDependenciesCount() = 0;
    virtual uint32_t getOutputDependenciesCount() = 0;
    virtual void getInputDependencies(NodeDependency* buffer) = 0;
    virtual void getOutputDependencies(NodeDependency* buffer) = 0;

    virtual void Record(ExecutionContext commandBuffer) = 0;

protected:
    RenderContext& context;
};