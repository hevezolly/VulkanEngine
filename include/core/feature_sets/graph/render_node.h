#pragma once
#include <common.h>
#include <allocator_feature.h>
#include <command_pool.h>
#include <resource_id.h>

enum struct SemaphoreRequirements {
    Timeline = 0,
    BinaryPerFrame = 1,
    BinaryPerSwapchainImage = 2
};

struct RenderContext;

struct API NodeDependency {
    ResourceId resource;
    ResourceState state;
    bool stateByWriter = false;
};

struct API ExecutionContext {
    TransferCommandBuffer* commandBuffer;
    MemBuffer<Ref<Semaphore>> executionStart;
};

struct API RenderNode {
    
    RenderNode(RenderContext& ctx): context(ctx), name(""){}

    virtual SemaphoreRequirements getSemaphoreRequirements() {return SemaphoreRequirements::Timeline;}
    virtual QueueType getTargetQueue() = 0;
    virtual uint32_t getInputDependenciesCount() = 0;
    virtual uint32_t getOutputDependenciesCount() = 0;
    virtual void getInputDependencies(NodeDependency* buffer) {}
    virtual void getOutputDependencies(NodeDependency* buffer) {}

    virtual void Record(ExecutionContext commandBuffer) = 0;

    void SetName(const std::string& newName) {
        name = newName;
    }

    std::string& getName() {
        return name;
    }

protected:
    RenderContext& context;
    std::string name;
};