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

    virtual QueueType queueType();

    void Begin();
    void End();
    void Reset();
};

struct API ComputeCommandBuffer: TransferCommandBuffer {
    virtual QueueType queueType();
};

struct API GraphicsCommandBuffer: ComputeCommandBuffer {
    virtual QueueType queueType();
    
    void BeginRenderPass(Ref<GraphicsPipeline> pipeline, Ref<FrameBuffer> frameBuffer);
    void EndRenderPass();
};

struct API CommandPool: FeatureSet, 
    CanHandle<InitMsg>,
    CanHandle<DestroyMsg>
{
    bool separateTransferPool;
    VkCommandPool graphicsCommandPool;
    VkCommandPool compueCommandPool;
    VkCommandPool transferCommandPool;
    
    CommandPool(RenderContext&);

    virtual void OnMessage(InitMsg*);
    virtual void OnMessage(DestroyMsg*);

    GraphicsCommandBuffer CreateGraphicsBuffer();
    TransferCommandBuffer CreateTransferBuffer();

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