#pragma once
#include <common.h>
#include <feature_set.h>
#include <device.h>
#include <graphics_feature.h>
#include <synchronization.h>
#include <messages.h>

struct API CommandBuffer {
    VkCommandBuffer buffer;
    void Begin();
    void End();
    void Reset();
    void BeginRenderPass(Ref<GraphicsPipeline> pipeline, Ref<FrameBuffer> frameBuffer);
    void EndRenderPass();
};

struct API CommandPool: FeatureSet, 
    CanHandle<InitMsg>,
    CanHandle<DestroyMsg>
{
    
    CommandPool(RenderContext&);

    virtual void OnMessage(InitMsg*);
    virtual void OnMessage(DestroyMsg*);

    CommandBuffer CreateGraphicsBuffer();

    VkCommandPool graphicsCommandPool;

    void Submit(CommandBuffer&, Ref<Semaphore> start, Ref<Semaphore> end, Ref<Fence>);
};