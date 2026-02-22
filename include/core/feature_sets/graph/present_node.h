#pragma once

#include <render_node.h>
#include <resource_storage.h>

struct PresentNode: RenderNode {

    PresentNode(
        RenderContext& c,
        ResourceRef<Image> presentImage
    );

    PresentNode(
        RenderContext& c,
        uint32_t presentImage
    );

    virtual QueueType getTargetQueue() {
        return QueueType::Present;
    }

    virtual uint32_t getInputDependenciesCount() {
        return 1;
    }

    virtual uint32_t getOutputDependenciesCount() {
        return 0;
    }

    virtual SemaphoreRequirements getSemaphoreRequirements() {return SemaphoreRequirements::BinaryPerSwapchainImage;}

    virtual void getInputDependencies(NodeDependency* buffer);

    virtual void Record(ExecutionContext context);

private:
    uint32_t presentImgIndex;
    ResourceRef<Image> presentImg;
};