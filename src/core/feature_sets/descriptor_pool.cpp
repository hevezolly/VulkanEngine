#include <descriptor_pool.h>
#include <render_context.h>

DescriptorPool::DescriptorPool(
    RenderContext* c, 
    std::initializer_list<VkDescriptorPoolSize> sizes, 
    bool createFree
): context(c) {
    VkDescriptorPoolCreateInfo info {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    info.poolSizeCount = sizes.size();
    info.pPoolSizes = sizes.begin();
    info.maxSets = 0;
    for (auto size: sizes) {
        info.maxSets += size.descriptorCount;
    }
    info.flags = createFree ? VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT : 0;

    VK(vkCreateDescriptorPool(c->device(), &info, nullptr, &vkPool));
}

DescriptorPool::~DescriptorPool() {
    if (!context)
        return;

    vkDestroyDescriptorPool(context->device(), vkPool, nullptr);

    context = nullptr;
}

DescriptorPool& DescriptorPool::operator=(DescriptorPool&& other) noexcept {
    if (&other == this)
        return *this;

    context = other.context;
    vkPool = other.vkPool;
    other.context = nullptr;
    other.vkPool = VK_NULL_HANDLE;

    return *this;
}

DescriptorPool::DescriptorPool(DescriptorPool&& other) noexcept {
    *this = std::move(other);
}