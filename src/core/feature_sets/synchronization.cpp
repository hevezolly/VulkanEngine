#include <synchronization.h>
#include <render_context.h>
#include <present_feature.h>
#include <swap_chain.h>

Semaphore::Semaphore(RenderContext* ctx): context(ctx){}
Fence::Fence(RenderContext* ctx): context(ctx){}

Semaphore::~Semaphore() {
    if (context) {
        vkDestroySemaphore(context->device(), vk, nullptr);
    context = nullptr;
    }
}

void Fence::Wait(uint64_t timeout) {
    vkWaitForFences(context->device(), 1, &vk, VK_TRUE, timeout);
}

void Fence::Reset() {
    vkResetFences(context->device(), 1, &vk);
}

Fence::~Fence() {
    if (context) {
        vkDestroyFence(context->device(), vk, nullptr);
        context = nullptr;
    }
}

Ref<Semaphore> Synchronization::CreateSemaphore(bool timeline) {
    Ref<Semaphore> s = context.New<Semaphore>(&context);
    s->binary = !timeline;
    VkSemaphoreCreateInfo info {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    VkSemaphoreTypeCreateInfo timelineCreateInfo;
    if (timeline) {
        timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
        timelineCreateInfo.pNext = nullptr;
        timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
        timelineCreateInfo.initialValue = 0;
        info.pNext = &timelineCreateInfo;
    }

    VK(vkCreateSemaphore(context.device(), &info, nullptr, &s->vk))
    return s;
}

Ref<Fence> Synchronization::CreateFence(bool signaled) {
    Ref<Fence> s = context.New<Fence>(&context);
    VkFenceCreateInfo info {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    if (signaled)
        info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VK(vkCreateFence(context.device(), &info, nullptr, &s->vk))
    return s;
}

Refs<Semaphore> Synchronization::CreateSemaphores(uint32_t size) {
    Refs<Semaphore> s = context.NewRefs<Semaphore>(size, &context);

    VkSemaphoreCreateInfo info {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    for (int i = 0; i < size; i++) {
        VK(vkCreateSemaphore(context.device(), &info, nullptr, &(s[i]->vk)))
    }
    return s;
}

Refs<Fence> Synchronization::CreateFences(uint32_t size, bool signaled) {
    Refs<Fence> s = context.NewRefs<Fence>(size, &context);
    
    VkFenceCreateInfo info {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    if (signaled)
        info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < size; i++) {
        VK(vkCreateFence(context.device(), &info, nullptr, &(s[i]->vk)))
    }
    return s;
}

void Synchronization::OnMessage(BeginFrameMsg* m) {
    _semaphoresPool.SetFrameWithReset(m->inFlightFrame);
}

void Synchronization::OnMessage(DestroyMsg*) {
    _semaphoresPool.clear();
}

void Synchronization::OnMessage(ImageAquiredMsg* m) {
    _semaphoresPerSwapChainImg.SetFrameWithReset(m->imageIndex);
}

Ref<Semaphore> Synchronization::BorrowBinarySemaphore(bool perSwapchainImg) {

    FramedObjectPool<Ref<Semaphore>>* pool = &_semaphoresPool;
    if (perSwapchainImg) {
        ASSERT(context.Has<PresentFeature>());

        pool = &_semaphoresPerSwapChainImg;
    }

    if (pool->isEmpty()) {
        pool->Insert(CreateSemaphore(false));
    }

    return pool->BorrowAndForget();
}