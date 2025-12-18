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

struct API CommandPool: FeatureSet, CanHandle<InitMessage> {
    
    CommandPool(RenderContext&);

    virtual void OnMessage(InitMessage*);
    virtual void Destroy();

    CommandBuffer CreateGraphicsBuffer();

    VkCommandPool graphicsCommandPool;

    void Submit(CommandBuffer&, Ref<Semaphore> start, Ref<Semaphore> end, Ref<Fence>);
};