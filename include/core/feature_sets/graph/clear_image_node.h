#pragma once

#include <render_node.h>
#include <image.h>

struct ClearImageNode: RenderNode {
    ClearImageNode(
        RenderContext& ctx, 
        ResourceRef<Image> img): 
    RenderNode(ctx), image(img), clearValue(img->clearValue)
    {
        formatDepthStencilSupport(image->description.format, depthSupport, stencilSupport);
        if (isDepthImage()){
            assignAspects(VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
        }
    }

    virtual QueueType getTargetQueue() {return QueueType::Graphics;}
    virtual uint32_t getInputDependenciesCount() {return 0;}
    virtual uint32_t getOutputDependenciesCount() {return 0;}

    void SetClearColor(VkClearColorValue clearValue){
        ASSERT(!isDepthImage());
        this->clearValue.color = clearValue;
    }

    void SetClearDepthStencil(VkClearDepthStencilValue clearValue, 
        VkImageAspectFlags aspects = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT){
        ASSERT(isDepthImage());
        assignAspects(aspects);
        ASSERT(dsAspects > 0);
        this->clearValue.depthStencil = clearValue;
    }

    virtual void getOutputDependencies(NodeDependency* buffer) {}

    virtual void Record(ExecutionContext commandBuffer) = 0;

private:
    bool depthSupport;
    bool stencilSupport;
    VkImageAspectFlags dsAspects;

    void assignAspects(VkImageAspectFlags aspects) {
        if (!depthSupport) {
            aspects &= ~VK_IMAGE_ASPECT_DEPTH_BIT;
        } 
        if (!stencilSupport) {
            aspects &= ~VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        dsAspects = aspects;
    }
    bool isDepthImage() {return depthSupport || stencilSupport;}
    VkClearValue clearValue;
    ResourceRef<Image> image;
};