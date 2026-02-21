#pragma once
#include <common.h>
#include <feature_set.h>
#include <device.h>
#include <graphics_feature.h>
#include <synchronization.h>
#include <messages.h>
#include <frame_buffer.h>
#include <resource_id.h>
#include <resource_storage.h>
#include <framed_object_pool.h>

struct API TransferCommandBuffer {
    VkCommandBuffer buffer;
    
    TransferCommandBuffer(VkCommandBuffer buffer, RenderContext* context);
    
    virtual QueueType queueType();

    void Begin();
    void End();
    void Reset();
    void CopyBufferRegion(VkBuffer src, VkBuffer dst, uint32_t size, uint32_t src_offset = 0, uint32_t dst_offset = 0);
    void ImageBarrier(ResourceRef<Image> img, const ResourceState& newState);
    void Barrier(uint32_t count, const ResourceId* ids, const ResourceState* states);
    
    friend struct CommandPool;
    
protected:
    RenderContext* context;
};

struct API ComputeCommandBuffer: TransferCommandBuffer {
    using TransferCommandBuffer::TransferCommandBuffer;
    virtual QueueType queueType();
    friend struct CommandPool;
};

struct API GraphicsCommandBuffer: ComputeCommandBuffer {
    using ComputeCommandBuffer::ComputeCommandBuffer;
    virtual QueueType queueType();
    
    void BeginRenderPass(Ref<GraphicsPipeline> pipeline, const FrameBuffer& frameBuffer);
    void EndRenderPass();
    friend struct CommandPool;
};

struct API CommandPool: FeatureSet, 
    CanHandle<InitMsg>,
    CanHandle<DestroyMsg>,
    CanHandle<BeginFrameMsg>
{
    VkCommandPool graphicsCommandPool;
    VkCommandPool compueCommandPool;
    VkCommandPool transferCommandPool;
    
    CommandPool(RenderContext&);

    virtual void OnMessage(InitMsg*);
    virtual void OnMessage(DestroyMsg*);
    virtual void OnMessage(BeginFrameMsg*);

    GraphicsCommandBuffer CreateGraphicsBuffer();
    TransferCommandBuffer CreateTransferBuffer(bool transient);

    TransferCommandBuffer BorrowCommandBuffer(QueueType queue);

    void Submit(
        TransferCommandBuffer&, 
        std::initializer_list<std::pair<Ref<Semaphore>, VkPipelineStageFlags>> start = {},
        std::initializer_list<Ref<Semaphore>> end = {},
        Ref<Fence> = Ref<Fence>::Null());

    void Submit(
        TransferCommandBuffer&,
        uint32_t numWaitSemaphores,
        VkSemaphoreSubmitInfo* waitSemaphores,
        uint32_t numSignamSemaphores,
        VkSemaphoreSubmitInfo* signalSemaphores,
        Ref<Fence> = Ref<Fence>::Null()
    );

private:
    FramedStorage<std::vector<VkCommandPool>> framedCommandPools;
    
    std::vector<FramedObjectPool<VkCommandBuffer>> allocatedBuffers;

    void Submit(
        TransferCommandBuffer&,
        uint32_t numWaitSemaphores,
        VkSemaphore* waitSemaphores,
        VkPipelineStageFlags* waitStages,
        uint32_t numSignamSemaphores,
        VkSemaphore* signalSemaphore,
        Ref<Fence>
    );
};