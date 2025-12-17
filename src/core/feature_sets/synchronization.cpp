#include <synchronization.h>
#include <render_context.h>

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

Ref<Semaphore> Synchronization::CreateSemaphore() {
    Ref<Semaphore> s = context.New<Semaphore>();
    s->context = &context;

    VkSemaphoreCreateInfo info {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VK(vkCreateSemaphore(context.device(), &info, nullptr, &s->vk))
    return s;
}

Ref<Fence> Synchronization::CreateFence(bool signaled) {
    Ref<Fence> s = context.New<Fence>();
    s -> context = &context;
    VkFenceCreateInfo info {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    if (signaled)
        info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VK(vkCreateFence(context.device(), &info, nullptr, &s->vk))
    return s;
}

