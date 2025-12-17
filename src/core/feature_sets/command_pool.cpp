#include <command_pool.h>
#include <render_context.h>
#include <graphics_feature.h>
#include <object_pool.h>

VkCommandPool CreateCommandPool(VkDevice device, uint32_t queueFamily) {
    VkCommandPool pool;

    VkCommandPoolCreateInfo info {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    info.queueFamilyIndex = queueFamily;

    VK(vkCreateCommandPool(device, &info, nullptr, &pool));

    return pool;
}

CommandPool::CommandPool(RenderContext& context): FeatureSet(context) {
}

void CommandPool::Init() {

    graphicsCommandPool = VK_NULL_HANDLE;
    
    if (context.Has<GraphicsFeature>()) {
        graphicsCommandPool = CreateCommandPool(
            context.device(), 
            context.Get<Device>().queueFamilies.get(QueueType::Graphics));
    };
}

void CommandPool::Destroy() {
    if (graphicsCommandPool != VK_NULL_HANDLE)
        vkDestroyCommandPool(context.device(), graphicsCommandPool, nullptr);
}

void CommandPool::Submit(
    CommandBuffer& buffer, 
    Ref<Semaphore> start, 
    Ref<Semaphore> end, 
    Ref<Fence> endFence
) {
    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &start->vk;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &end->vk;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &buffer.buffer;

    VK(vkQueueSubmit(context.Get<Device>().queues.get(QueueType::Graphics), 1, &submitInfo, endFence->vk));
}

CommandBuffer CommandPool::CreateGraphicsBuffer() {
    CommandBuffer buffer;

    VkCommandBufferAllocateInfo info {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    info.commandPool = graphicsCommandPool;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = 1;

    VK(vkAllocateCommandBuffers(context.device(), &info, &buffer.buffer));

    return buffer;
}

void CommandBuffer::Begin() {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    VK(vkBeginCommandBuffer(buffer, &beginInfo));
}

void CommandBuffer::End() {
    VK(vkEndCommandBuffer(buffer));
}

void CommandBuffer::Reset() {
    vkResetCommandBuffer(buffer, 0);
}

void CommandBuffer::BeginRenderPass(Ref<GraphicsPipeline> pipeline, Ref<FrameBuffer> frameBuffer) {
    VkRenderPassBeginInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    renderPassInfo.renderPass = pipeline->renderPass;
    renderPassInfo.framebuffer = frameBuffer->frameBuffer;

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = {frameBuffer->width, frameBuffer->height};

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
}

void CommandBuffer::EndRenderPass() {
    vkCmdEndRenderPass(buffer);
}