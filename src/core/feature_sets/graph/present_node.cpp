#include <present_node.h>
#include <render_context.h>
#include <present_feature.h>


PresentNode::PresentNode(RenderContext& c, ResourceRef<Image> presentImage): 
    RenderNode(c), presentImg(presentImage) 
{
    for (;presentImgIndex < context.Get<PresentFeature>().swapChain->images.size(); presentImgIndex++) {

        if (context.Get<PresentFeature>().swapChain->images[presentImgIndex].id == presentImage.id)
            break;
    }

    if (presentImgIndex == context.Get<PresentFeature>().swapChain->images.size()) {
        throw std::runtime_error("image provided to PresentNode is not from the swapchain");
    }
}

PresentNode::PresentNode(RenderContext& c, uint32_t index): 
    RenderNode(c), presentImgIndex(index) 
{
    presentImg = context.Get<PresentFeature>().swapChain->images[index];
}

void PresentNode::getInputDependencies(NodeDependency* buffer) {
    buffer->resource = presentImg.id;
    buffer->state = ResourceState {
        0,
        0,
        VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };
    buffer->stateByWriter = true;
}

void PresentNode::Record(ExecutionContext executionContext) {
    auto _ = context.Get<Allocator>().BeginContext();
    MemBuffer<VkSemaphore> semaphores = context.Get<Allocator>()
        .BumpAllocate<VkSemaphore>(executionContext.executionStart.size());

    for (int i = 0; i < semaphores.capacity(); i++) {
        semaphores.push_back(executionContext.executionStart[i]->vk);
    }

    context.Get<PresentFeature>().Present(presentImgIndex, semaphores.size(), semaphores.data());
}