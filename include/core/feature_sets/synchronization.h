#pragma once
#include <feature_set.h>
#include <handles.h>

struct API Semaphore {
    VkSemaphore vk;

    ~Semaphore();

    friend struct Synchronization;
private:
    RenderContext* context;
};

struct API Fence {
    VkFence vk;

    void Wait(uint64_t timeout = UINT64_MAX);
    void Reset();

    ~Fence();

    friend struct Synchronization;
private:
    RenderContext* context;
};

struct API Synchronization: FeatureSet {
    using FeatureSet::FeatureSet;

    Ref<Semaphore> CreateSemaphore();
    Ref<Fence> CreateFence(bool signaled=false);
};