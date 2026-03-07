#include <command_pool.h>
#include <render_context.h>
#include <graphics_feature.h>
#include <allocator_feature.h>
#include <resources.h>

TransferCommandBuffer::TransferCommandBuffer(VkCommandBuffer b, RenderContext* c):
    context(c),
    buffer(b)
{}

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

void createCommandPoolsSet(
    RenderContext& context,
    VkCommandPool& graphicsCommandPool,
    VkCommandPool& compueCommandPool,
    VkCommandPool& transferCommandPool,
    const std::string& nameSuffix = ""
) {
    graphicsCommandPool = VK_NULL_HANDLE;
    compueCommandPool = VK_NULL_HANDLE;
    
    uint32_t transferQueue = context.Get<Device>().queueFamilies.get(QueueType::Transfer);

    if (context.Has<GraphicsFeature>()) {
        uint32_t graphicsQueue = context.Get<Device>().queueFamilies.get(QueueType::Graphics);
        graphicsCommandPool = CreateCommandPool(
            context.device(), 
            graphicsQueue,
            false
        );
    }

    transferCommandPool = CreateCommandPool(
        context.device(),
        transferQueue,
        false
    );
}

void CommandPool::OnMessage(InitMsg* m) {

    createCommandPoolsSet(context, graphicsCommandPool, compueCommandPool, transferCommandPool);
    
}

void CommandPool::OnMessage(DestroyMsg* m) {
    if (graphicsCommandPool != VK_NULL_HANDLE)
        vkDestroyCommandPool(context.device(), graphicsCommandPool, nullptr);

    vkDestroyCommandPool(context.device(), transferCommandPool, nullptr);

    for(uint32_t frame = 0; frame < framedCommandPools.size(); frame++) {
        for (uint32_t queue = 0; queue < framedCommandPools.get(frame).size(); queue++) {
            VkCommandPool pool = framedCommandPools.get(frame)[queue];
            if (pool != VK_NULL_HANDLE)
                vkDestroyCommandPool(context.device(), pool, nullptr);
        }
    }
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

void CommandPool::Submit(
    TransferCommandBuffer& buffer,
    uint32_t numWaitSemaphores,
    VkSemaphoreSubmitInfo* waitSemaphores,
    uint32_t numSignamSemaphores,
    VkSemaphoreSubmitInfo* signalSemaphores,
    Ref<Fence> fence
) {
    VkFence vkfence = VK_NULL_HANDLE;
    if (!fence.isNull())
        vkfence = fence->vk;

    VkCommandBufferSubmitInfo cmdInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    cmdInfo.commandBuffer = buffer.buffer;

    VkSubmitInfo2 submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    submitInfo.waitSemaphoreInfoCount = numWaitSemaphores;
    submitInfo.pWaitSemaphoreInfos = waitSemaphores;
    submitInfo.signalSemaphoreInfoCount = numSignamSemaphores;
    submitInfo.pSignalSemaphoreInfos = signalSemaphores;
    submitInfo.commandBufferInfoCount = 1;
    submitInfo.pCommandBufferInfos = &cmdInfo;

    VK(vkQueueSubmit2(context.Get<Device>().queues.get(buffer.queueType()), 1, &submitInfo, vkfence));
}

GraphicsCommandBuffer CommandPool::CreateGraphicsBuffer() {
    VkCommandBuffer buffer;
    CreateBuffer(context.device(), graphicsCommandPool, &buffer);

    return GraphicsCommandBuffer(buffer, &context);
}

TransferCommandBuffer CommandPool::CreateTransferBuffer(bool transient) {

    if (transient)
        return *BorrowCommandBuffer(QueueType::Transfer);


    VkCommandBuffer buffer;
    VkCommandPool pool = transferCommandPool;
    CreateBuffer(context.device(), pool, &buffer);
    return TransferCommandBuffer(buffer, &context);
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

void GraphicsCommandBuffer::BeginRenderPass(Ref<GraphicsPipeline> pipeline, const FrameBuffer& frameBuffer) {
    assert(!currentPipeline.has_value());
    
    VkRenderPassBeginInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    renderPassInfo.renderPass = pipeline->renderPass;
    renderPassInfo.framebuffer = frameBuffer.frameBuffer;

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = {frameBuffer.width, frameBuffer.height};

    renderPassInfo.clearValueCount = frameBuffer.clearValues.size();
    renderPassInfo.pClearValues = frameBuffer.clearValues.data();

    vkCmdBeginRenderPass(buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
    currentPipeline = VK_PIPELINE_BIND_POINT_GRAPHICS;
}

void ComputeCommandBuffer::EndRenderPass() {
    assert(currentPipeline.has_value());
    vkCmdEndRenderPass(buffer);
    currentPipeline = std::nullopt;
}

void ComputeCommandBuffer::BindShaderInput(uint32_t count, const ShaderInputInstance* input) {
    assert(currentPipeline.has_value());
    
    uint32_t dynamicStateSize = 0;
    for (int i = 0; i < count; i++) {
        dynamicStateSize += input[i].dynamicOffsetsCount;
    }

    Allocator& alloc = context->Get<Allocator>();
    auto _ = alloc.BeginContext();
    MemChunk<uint32_t> dynamicStates = alloc.BumpAllocate<uint32_t>(dynamicStateSize);
    MemChunk<VkDescriptorSet> descriptorSets = alloc.BumpAllocate<VkDescriptorSet>(count);

    dynamicStateSize = 0;
    for (int i = 0; i < count; i++) {
        descriptorSets[i] = input[i].descriptor->vkSet;
        
        uint32_t dCount = input[i].dynamicOffsetsCount;
        if (dCount > 0) {
            std::memcpy(dynamicStates.data + dynamicStateSize, input[i].pDynamicOffsets, dCount * sizeof(uint32_t));
            dynamicStateSize += dCount;
        }
    }

    
}

void ComputeCommandBuffer::BindShaderInput(std::initializer_list<ShaderInputInstance> input) {
    BindShaderInput(static_cast<uint32_t>(input.size()), input.begin());
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

std::unique_ptr<TransferCommandBuffer> CommandPool::BorrowCommandBuffer(QueueType queue) {
    assert(queue == QueueType::Graphics || queue == QueueType::Compute || queue == QueueType::Transfer);

    if (queue == QueueType::Graphics && !context.Has<GraphicsFeature>())
        throw std::runtime_error("attempt to access graphics queue without GraphicsFeature enabled");
    
    uint32_t index = static_cast<uint32_t>(queue);
    if (framedCommandPools->size() <= index) {
        framedCommandPools->resize(3);
        createCommandPoolsSet(context, 
            framedCommandPools.val()[static_cast<uint32_t>(QueueType::Graphics)],
            framedCommandPools.val()[static_cast<uint32_t>(QueueType::Compute)],
            framedCommandPools.val()[static_cast<uint32_t>(QueueType::Transfer)]);
    }

    if (allocatedBuffers.size() <= index) {
        allocatedBuffers.resize(3);
    }

    if (allocatedBuffers[index].isEmpty()) {
        VkCommandBuffer buffer;
        CreateBuffer(context.device(), 
            framedCommandPools.val()[index],
            &buffer 
        );
        allocatedBuffers[index].Insert(std::move(buffer));
    }

    VkCommandBuffer result = allocatedBuffers[index].BorrowAndForget();

    switch (queue)
    {
    case QueueType::Graphics:
        return std::make_unique<GraphicsCommandBuffer>(result, &context);
    
    case QueueType::Compute:
        return std::make_unique<ComputeCommandBuffer>(result, &context);

    default:
        return std::make_unique<TransferCommandBuffer>(result, &context);
    }
}

void CommandPool::OnMessage(BeginFrameLateMsg* msg) {
    framedCommandPools.SetFrame(msg->inFlightFrame);
    for (int i = 0; i < framedCommandPools.val().size(); i++) {
        if (framedCommandPools.val()[i] != VK_NULL_HANDLE)
            vkResetCommandPool(context.device(), framedCommandPools.val()[i], 0);
    }

    for (int i = 0; i < allocatedBuffers.size(); i++) {
        allocatedBuffers[i].SetFrameWithReset(msg->inFlightFrame);
    }
}