#pragma once

#include <render_node.h>
#include <subresources.h>

struct ClearImageNode: RenderNode {
    ClearImageNode(RenderContext& ctx, ImageSubresource img);

    virtual QueueType getTargetQueue() {return QueueType::Graphics;}
    virtual uint32_t getInputDependenciesCount() {return 0;}
    virtual uint32_t getOutputDependenciesCount() {return 1;}

    void SetClearColor(VkClearColorValue clearValue){
        ASSERT(!isDepthImage);
        this->clearValue.color = clearValue;
    }

    void SetClearDepthStencil(VkClearDepthStencilValue clearValue, 
        VkImageAspectFlags aspects = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT){
        ASSERT(isDepthImage);
        dsAspects = image.range.aspectMask & aspects;
        ASSERT(dsAspects > 0);
        this->clearValue.depthStencil = clearValue;
    }

    virtual void getOutputDependencies(NodeDependency* buffer);

    virtual void Record(ExecutionContext commandBuffer);

private:
    VkImageAspectFlags dsAspects;
    bool isDepthImage;
    VkClearValue clearValue;
    ImageSubresource image;
};