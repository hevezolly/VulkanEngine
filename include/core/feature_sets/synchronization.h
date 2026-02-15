#pragma once
#include <common.h>
#include <feature_set.h>
#include <handles.h>
#include <allocator_feature.h>
#include <framed_object_pool.h>

struct API Semaphore {
    VkSemaphore vk;

    Semaphore(RenderContext*);
    ~Semaphore();

private:
    RenderContext* context;
};

struct API Fence {
    VkFence vk;

    Fence(RenderContext*);
    void Wait(uint64_t timeout = UINT64_MAX);
    void Reset();

    ~Fence();

private:
    RenderContext* context;
};

struct API Synchronization: FeatureSet, 
    CanHandle<BeginFrameMsg>, 
    CanHandle<DestroyMsg>   
{
    using FeatureSet::FeatureSet;

    void OnMessage(BeginFrameMsg*);
    void OnMessage(DestroyMsg*);

    Ref<Semaphore> CreateSemaphore(bool timeline = false);
    Refs<Semaphore> CreateSemaphores(uint32_t size);

    Ref<Semaphore> BorrowSemaphore(bool timeline);

    Ref<Fence> CreateFence(bool signaled=false);
    Refs<Fence> CreateFences(uint32_t size, bool signaled=false);

private:

    FramedObjectPool<Ref<Semaphore>> _semaphoresPool;
};