#include <command_pool.h>
#include <render_context.h>
#include <graphics_feature.h>

TransferCommandBuffer::~TransferCommandBuffer() {
    if (!transient || vkDevice == VK_NULL_HANDLE)
        return;

    vkFreeCommandBuffers(vkDevice, vkCommandPool, 1, &buffer);
}

TransferCommandBuffer::TransferCommandBuffer(VkCommandBuffer b, VkDevice device, VkCommandPool pool, bool t):
    transient(t),
    vkDevice(device),
    vkCommandPool(pool),
    buffer(b)
{}

TransferCommandBuffer& TransferCommandBuffer::operator=(TransferCommandBuffer&& other) noexcept {
    if (this == &other)
        return *this;

    buffer = other.buffer;
    vkDevice = other.vkDevice;
    vkCommandPool = other.vkCommandPool;
    transient = other.transient;

    other.buffer = VK_NULL_HANDLE;
    other.vkDevice = VK_NULL_HANDLE;
    other.vkCommandPool = VK_NULL_HANDLE;

    return *this;
}

void TransferCommandBuffer::CopyBufferRegion(
    VkBuffer src, 
    VkBuffer dst, 
    uint32_t size, 
    uint32_t src_offset, 
    uint32_t dst_offset
) {
    VkBufferCopy copy{};
    copy.srcOffset = src_offset;
    copy.dstOffset = dst_offset;
    copy.size = size;

    vkCmdCopyBuffer(buffer, src, dst, 1, &copy);
}

TransferCommandBuffer::TransferCommandBuffer(TransferCommandBuffer&& other) noexcept {
    *this = std::move(other);
}

QueueType TransferCommandBuffer::queueType() {
    return QueueType::Transfer;
}

QueueType ComputeCommandBuffer::queueType() {
    return QueueType::Compute;
}

QueueType GraphicsCommandBuffer::queueType() {
    return QueueType::Graphics;
}

VkCommandPool CreateCommandPool(VkDevice device, uint32_t queueFamily, bool transient) {
    VkCommandPool pool;

    VkCommandPoolCreateInfo info {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    info.flags = transient ? VK_COMMAND_POOL_CREATE_TRANSIENT_BIT : VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
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
    
    separateTransferQueue = true;
    uint32_t transferQueue = context.Get<Device>().queueFamilies.get(QueueType::Transfer);

    if (context.Has<GraphicsFeature>()) {
        uint32_t graphicsQueue = context.Get<Device>().queueFamilies.get(QueueType::Graphics);
        graphicsCommandPool = CreateCommandPool(
            context.device(), 
            graphicsQueue,
            false
        );
        separateTransferQueue &= graphicsQueue != transferQueue;
    }

    transferCommandPool = CreateCommandPool(
        context.device(),
        transferQueue,
        false
    );

    transientTransferCommandPool = CreateCommandPool(
        context.device(),
        transferQueue,
        true
    );
}

void CommandPool::OnMessage(DestroyMsg* m) {
    if (graphicsCommandPool != VK_NULL_HANDLE)
        vkDestroyCommandPool(context.device(), graphicsCommandPool, nullptr);

    vkDestroyCommandPool(context.device(), transferCommandPool, nullptr);
    vkDestroyCommandPool(context.device(), transientTransferCommandPool, nullptr);
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
    VkCommandBuffer buffer;
    CreateBuffer(context.device(), graphicsCommandPool, &buffer);

    return GraphicsCommandBuffer(buffer, context.device(), graphicsCommandPool, false);
}

TransferCommandBuffer CommandPool::CreateTransferBuffer(bool transient) {
    VkCommandBuffer buffer;
    VkCommandPool pool = transient ? transientTransferCommandPool : transferCommandPool;
    CreateBuffer(context.device(), pool, &buffer);
    return TransferCommandBuffer(buffer, context.device(), pool, transient);
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
    assert(!transient);
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

    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
}

void GraphicsCommandBuffer::EndRenderPass() {
    vkCmdEndRenderPass(buffer);
}

void TransferCommandBuffer::ImageBarrier(Image& img, VkImageLayout newLayout, VkAccessFlags access, VkPipelineStageFlags srcStage, VkPipelineStageFlags destStage) {

    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.oldLayout = img.state.currentLayout;
    barrier.newLayout = newLayout;

    barrier.image = img.vkImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    barrier.srcAccessMask = img.state.currentAccess;
    barrier.dstAccessMask = access;

    vkCmdPipelineBarrier(buffer,
        srcStage, destStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    img.state.currentAccess = access;
    img.state.currentLayout = newLayout;
}