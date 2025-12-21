#include <command_pool.h>
#include <render_context.h>
#include <graphics_feature.h>

QueueType TransferCommandBuffer::queueType() {
    return QueueType::Graphics;
}

QueueType ComputeCommandBuffer::queueType() {
    return QueueType::Compute;
}

QueueType GraphicsCommandBuffer::queueType() {
    return QueueType::Graphics;
}

VkCommandPool CreateCommandPool(VkDevice device, uint32_t queueFamily) {
    VkCommandPool pool;

    VkCommandPoolCreateInfo info {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    info.queueFamilyIndex = queueFamily;

    VK(vkCreateCommandPool(device, &info, nullptr, &pool));

    return pool;
}

void CreateBuffer(VkDevice device, VkCommandPool pool, VkCommandBuffer* buffer) {
    VkCommandBufferAllocateInfo info {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    info.commandPool = pool;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = 1;

    VK(vkAllocateCommandBuffers(device, &info, buffer));
}

CommandPool::CommandPool(RenderContext& context): FeatureSet(context) {
}

void CommandPool::OnMessage(InitMsg* m) {

    graphicsCommandPool = VK_NULL_HANDLE;
    compueCommandPool = VK_NULL_HANDLE;
    
    separateTransferPool = true;
    uint32_t transferQueue = context.Get<Device>().queueFamilies.get(QueueType::Transfer);

    if (context.Has<GraphicsFeature>()) {
        uint32_t graphicsQueue = context.Get<Device>().queueFamilies.get(QueueType::Graphics);
        graphicsCommandPool = CreateCommandPool(
            context.device(), 
            graphicsQueue
        );
        
        if (separateTransferPool && graphicsQueue == transferQueue)
            transferCommandPool = graphicsCommandPool;
        
        separateTransferPool &= graphicsQueue != transferQueue;
    }

    if (separateTransferPool) {
        transferCommandPool = CreateCommandPool(
            context.device(),
            transferQueue
        );
    }
}

void CommandPool::OnMessage(DestroyMsg* m) {
    if (graphicsCommandPool != VK_NULL_HANDLE)
        vkDestroyCommandPool(context.device(), graphicsCommandPool, nullptr);

    if (separateTransferPool)
        vkDestroyCommandPool(context.device(), transferCommandPool, nullptr);
}

void CommandPool::Submit(
    TransferCommandBuffer& buffer, 
    std::initializer_list<std::pair<Ref<Semaphore>, VkPipelineStageFlags>> start,
    std::initializer_list<Ref<Semaphore>> end,
    Ref<Fence> fence
) {
    uint32_t sizeWait = start.size();
    uint32_t sizeSignal = end.size();
    std::vector<VkSemaphore> waitSemaphores{sizeWait};
    std::vector<VkPipelineStageFlags> waitStages{sizeWait};
    std::vector<VkSemaphore> signalSemaphores{sizeSignal};

    int index = 0;
    for (auto p: start) {
        waitSemaphores[index] = p.first->vk;
        waitStages[index] = p.second;
        index++;
    }

    index = 0;
    for (auto s: end) {
        signalSemaphores[index++] = s->vk;
    }

    Submit(buffer, sizeWait, waitSemaphores.data(), waitStages.data(), sizeSignal, signalSemaphores.data(), fence);
}

void CommandPool::Submit(
    TransferCommandBuffer& buffer,
    uint32_t numWaitSemaphores,
    VkSemaphore* waitSemaphores,
    VkPipelineStageFlags* waitStages,
    uint32_t numSignamSemaphores,
    VkSemaphore* signalSemaphore,
    Ref<Fence> endFence
) {
    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    
    submitInfo.waitSemaphoreCount = numWaitSemaphores;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.signalSemaphoreCount = numSignamSemaphores;
    submitInfo.pSignalSemaphores = signalSemaphore;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &buffer.buffer;

    VkFence fence = VK_NULL_HANDLE;
    if (!endFence.isNull())
        fence = endFence->vk;

    VK(vkQueueSubmit(context.Get<Device>().queues.get(buffer.queueType()), 1, &submitInfo, fence));
}

GraphicsCommandBuffer CommandPool::CreateGraphicsBuffer() {
    GraphicsCommandBuffer buffer;
    CreateBuffer(context.device(), graphicsCommandPool, &buffer.buffer);
    return buffer;
}

TransferCommandBuffer CommandPool::CreateTransferBuffer() {
    TransferCommandBuffer buffer;
    CreateBuffer(context.device(), transferCommandPool, &buffer.buffer);
    return buffer;
}

void TransferCommandBuffer::Begin() {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    VK(vkBeginCommandBuffer(buffer, &beginInfo));
}

void TransferCommandBuffer::End() {
    VK(vkEndCommandBuffer(buffer));
}

void TransferCommandBuffer::Reset() {
    vkResetCommandBuffer(buffer, 0);
}

void GraphicsCommandBuffer::BeginRenderPass(Ref<GraphicsPipeline> pipeline, Ref<FrameBuffer> frameBuffer) {
    VkRenderPassBeginInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    renderPassInfo.renderPass = pipeline->renderPass;
    renderPassInfo.framebuffer = frameBuffer->frameBuffer;

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = {frameBuffer->width, frameBuffer->height};

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    LOG("render pass begin internal")

    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
    LOG("render pass bind pipeline")
}

void GraphicsCommandBuffer::EndRenderPass() {
    vkCmdEndRenderPass(buffer);
}