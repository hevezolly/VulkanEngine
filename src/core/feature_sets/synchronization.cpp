#include <synchronization.h>
#include <render_context.h>

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

    VkSemaphoreCreateInfo info {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    VkSemaphoreTypeCreateInfo timelineCreateInfo;
    if (timeline) {
        timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
        timelineCreateInfo.pNext = NULL;
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
    _borrowedSemaphores.SetFrame(m->inFlightFrame);
    _borrowedSemaphores->clear();
}

void Synchronization::OnMessage(DestroyMsg*) {
    _borrowedSemaphores.clearAll();
}

Ref<Semaphore> Synchronization::BorrowSemaphore() {
    if (semaphoresPool.isEmpty()) {
        semaphoresPool.Insert(CreateSemaphore());
    }

    _borrowedSemaphores->push_back(semaphoresPool.Borrow());
    return *_borrowedSemaphores->back();
}

void Synchronization::ReturnSemaphorEarly(Ref<Semaphore> s) {
    for (int i = _borrowedSemaphores->size() -1; i >= 0; i--) {
        if (_borrowedSemaphores[i].val() == s) {
            _borrowedSemaphores->erase(_borrowedSemaphores->begin() + i);
            break;
        }
    }
}