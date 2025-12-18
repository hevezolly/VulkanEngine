#pragma once
#include <feature_set.h>
#include <handles.h>

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

struct API Synchronization: FeatureSet {
    using FeatureSet::FeatureSet;

    Ref<Semaphore> CreateSemaphore();
    Refs<Semaphore> CreateSemaphores(uint32_t size);

    Ref<Fence> CreateFence(bool signaled=false);
    Refs<Fence> CreateFences(uint32_t size, bool signaled=false);
};