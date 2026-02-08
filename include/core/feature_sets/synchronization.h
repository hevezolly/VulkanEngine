#pragma once
#include <feature_set.h>
#include <handles.h>
#include <allocator_feature.h>
#include <framed_storage.h>
#include <object_pool.h>

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

    Ref<Semaphore> BorrowSemaphore();
    void ReturnSemaphorEarly(Ref<Semaphore> s);

    Ref<Fence> CreateFence(bool signaled=false);
    Refs<Fence> CreateFences(uint32_t size, bool signaled=false);

private:
    FramedStorage<Borrowed<Ref<Semaphore>>> _borrowedSemaphores;
    ObjectPool<Ref<Semaphore>> semaphoresPool;
};