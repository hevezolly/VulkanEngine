#pragma once
#include <common.h>
#include <feature_set.h>
#include <device.h>
#include <graphics_feature.h>
#include <synchronization.h>
#include <messages.h>
#include <frame_buffer.h>

struct API TransferCommandBuffer {
    VkCommandBuffer buffer;
    
    TransferCommandBuffer(VkCommandBuffer buffer, VkDevice device, VkCommandPool pool, bool transient);
    
    virtual QueueType queueType();

    void Begin();
    void End();
    void Reset();
    void CopyBufferRegion(VkBuffer src, VkBuffer dst, uint32_t size, uint32_t src_offset = 0, uint32_t dst_offset = 0);

    virtual RULE_5(TransferCommandBuffer)
    
    friend struct CommandPool;
    
protected:
    bool transient;
    VkDevice vkDevice;
    VkCommandPool vkCommandPool;
};

struct API ComputeCommandBuffer: TransferCommandBuffer {
    using TransferCommandBuffer::TransferCommandBuffer;
    virtual QueueType queueType();
    friend struct CommandPool;
};

struct API GraphicsCommandBuffer: ComputeCommandBuffer {
    using ComputeCommandBuffer::ComputeCommandBuffer;
    virtual QueueType queueType();
    
    void BeginRenderPass(Ref<GraphicsPipeline> pipeline, Ref<FrameBuffer> frameBuffer);
    void EndRenderPass();
    friend struct CommandPool;
};

struct API CommandPool: FeatureSet, 
    CanHandle<InitMsg>,
    CanHandle<DestroyMsg>
{
    bool separateTransferQueue;
    VkCommandPool graphicsCommandPool;
    VkCommandPool compueCommandPool;
    VkCommandPool transferCommandPool;
    VkCommandPool transientTransferCommandPool;
    
    CommandPool(RenderContext&);

    virtual void OnMessage(InitMsg*);
    virtual void OnMessage(DestroyMsg*);

    GraphicsCommandBuffer CreateGraphicsBuffer();
    TransferCommandBuffer CreateTransferBuffer(bool transient);

    void Submit(
        TransferCommandBuffer&, 
        std::initializer_list<std::pair<Ref<Semaphore>, VkPipelineStageFlags>> start = {},
        std::initializer_list<Ref<Semaphore>> end = {},
        Ref<Fence> = Ref<Fence>::Null());

private: 
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