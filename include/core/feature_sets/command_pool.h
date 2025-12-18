#pragma once
#include <common.h>
#include <feature_set.h>
#include <device.h>
#include <graphics_feature.h>
#include <synchronization.h>

struct API CommandBuffer {
    VkCommandBuffer buffer;
    void Begin();
    void End();
    void Reset();
    void BeginRenderPass(Ref<GraphicsPipeline> pipeline, Ref<FrameBuffer> frameBuffer);
    void EndRenderPass();
};

struct API CommandPool: FeatureSet {
    
    CommandPool(RenderContext&);

    virtual void Init();
    virtual void Destroy();

    CommandBuffer CreateGraphicsBuffer();

    VkCommandPool graphicsCommandPool;

    void Submit(CommandBuffer&, Ref<Semaphore> start, Ref<Semaphore> end, Ref<Fence>);
};