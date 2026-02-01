#include <command_pool.h>
#include <render_context.h>
#include <graphics_feature.h>
#include <allocator_feature.h>
#include <resources.h>

TransferCommandBuffer::~TransferCommandBuffer() {
    if (!transient || context==nullptr)
        return;

    vkFreeCommandBuffers(context->device(), vkCommandPool, 1, &buffer);
}

TransferCommandBuffer::TransferCommandBuffer(VkCommandBuffer b, RenderContext* c, VkCommandPool pool, bool t):
    transient(t),
    context(c),
    vkCommandPool(pool),
    buffer(b)
{}

TransferCommandBuffer& TransferCommandBuffer::operator=(TransferCommandBuffer&& other) noexcept {
    if (this == &other)
        return *this;

    buffer = other.buffer;
    context = other.context;
    vkCommandPool = other.vkCommandPool;
    transient = other.transient;

    other.buffer = VK_NULL_HANDLE;
    other.context = nullptr;
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

    return GraphicsCommandBuffer(buffer, &context, graphicsCommandPool, false);
}

TransferCommandBuffer CommandPool::CreateTransferBuffer(bool transient) {
    VkCommandBuffer buffer;
    VkCommandPool pool = transient ? transientTransferCommandPool : transferCommandPool;
    CreateBuffer(context.device(), pool, &buffer);
    return TransferCommandBuffer(buffer, &context, pool, transient);
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

void GraphicsCommandBuffer::BeginRenderPass(Ref<GraphicsPipeline> pipeline, const FrameBuffer& frameBuffer) {
    VkRenderPassBeginInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    renderPassInfo.renderPass = pipeline->renderPass;
    renderPassInfo.framebuffer = frameBuffer.frameBuffer;

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = {frameBuffer.width, frameBuffer.height};

    renderPassInfo.clearValueCount = frameBuffer.clearValues.size();
    renderPassInfo.pClearValues = frameBuffer.clearValues.data();

    vkCmdBeginRenderPass(buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
}

void GraphicsCommandBuffer::EndRenderPass() {
    vkCmdEndRenderPass(buffer);
}

void TransferCommandBuffer::ImageBarrier(ResourceRef<Image> img, const ResourceState& newState) {
    Barrier(1, &img.id, &newState);
}

void TransferCommandBuffer::Barrier(uint32_t count, const ResourceId* ids, const ResourceState* states) {

    if (count == 0)
        return;

    Allocator& alloc = context->Get<Allocator>();
    Resources& resources = context->Get<Resources>();
    auto _ = alloc.BeginContext();

    MemBuffer<VkImageMemoryBarrier2> imageBarriers = alloc.BumpAllocate<VkImageMemoryBarrier2>(count);
    MemBuffer<VkBufferMemoryBarrier2> bufferBarriers = alloc.BumpAllocate<VkBufferMemoryBarrier2>(count);

    for (int i = 0; i < count; i++) {

        ResourceId id = ids[i];
        ResourceState newState = states[i];
        ResourceState oldState = resources.GetState(id);

        if (id.type() == ResourceType::Image) {
            imageBarriers.push_back(VkImageMemoryBarrier2{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2});

            imageBarriers.back().srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers.back().dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            imageBarriers.back().oldLayout = oldState.currentLayout;
            imageBarriers.back().newLayout = newState.currentLayout;

            auto image = resources.Get<Image>(id);
            imageBarriers.back().image = image->vkImage;
            imageBarriers.back().subresourceRange =  image->view->subresourceRange;

            imageBarriers.back().srcAccessMask = oldState.currentAccess;
            imageBarriers.back().dstAccessMask = newState.currentAccess;

            imageBarriers.back().srcStageMask = oldState.accessStage;
            imageBarriers.back().dstStageMask = newState.accessStage;

            resources.SetState(id, newState);

        } else if (id.type() == ResourceType::Buffer) {
            bufferBarriers.push_back(VkBufferMemoryBarrier2{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2});

            ResourceRef<Buffer> buffer = resources.Get<Buffer>(id);

            bufferBarriers.back().offset = buffer->memory.offset;
            bufferBarriers.back().size = buffer->size_bytes();

            bufferBarriers.back().srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufferBarriers.back().dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            bufferBarriers.back().srcAccessMask = oldState.currentAccess;
            bufferBarriers.back().dstAccessMask = newState.currentAccess;

            bufferBarriers.back().srcStageMask = oldState.accessStage;
            bufferBarriers.back().dstStageMask = newState.accessStage;

            bufferBarriers.back().buffer = buffer->vkBuffer;

            resources.SetState(id, newState);
        }
    }

    if (imageBarriers.size() == 0 && bufferBarriers.size() == 0)
        return;

    VkDependencyInfo dep{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dep.imageMemoryBarrierCount = imageBarriers.size();
    dep.pImageMemoryBarriers = imageBarriers.data();

    dep.bufferMemoryBarrierCount = bufferBarriers.size();
    dep.pBufferMemoryBarriers = bufferBarriers.data();

    vkCmdPipelineBarrier2(buffer, &dep);
}