#include <descriptor_pool.h>
#include <render_context.h>

DescriptorPool::DescriptorPool(
    RenderContext* c, 
    uint32_t count,
    const VkDescriptorPoolSize* sizes, 
    bool createFree
): context(c) {
    VkDescriptorPoolCreateInfo info {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    info.poolSizeCount = count;
    info.pPoolSizes = sizes;
    info.maxSets = 0;
    for (int i = 0; i < count; i++) {
        info.maxSets += sizes[i].descriptorCount;
    }
    info.flags = createFree ? VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT : 0;

    VK(vkCreateDescriptorPool(c->device(), &info, nullptr, &vkPool));
}

DescriptorPool::DescriptorPool(
    RenderContext* c, 
    std::initializer_list<VkDescriptorPoolSize> sizes, 
    bool createFree
): DescriptorPool(c, static_cast<uint32_t>(sizes.size()), sizes.begin(), createFree)
{}

DescriptorPool::~DescriptorPool() {
    if (!context)
        return;

    vkDestroyDescriptorPool(context->device(), vkPool, nullptr);

    context = nullptr;
}

DescriptorSet& DescriptorSet::operator=(DescriptorSet&& other) noexcept {
    if (&other == this)
        return *this;

    vkSet = other.vkSet;
    pool = other.pool;
    context = other.context;

    other.pool = nullptr;
    other.vkSet = VK_NULL_HANDLE;
    other.context = nullptr;

    return *this;
}

DescriptorSet::DescriptorSet(DescriptorSet&& other) noexcept {
    *this = std::move(other);
}

SpecializedDescriptorPool::~SpecializedDescriptorPool() {
    assert(availableInstances == maxInstances);
}

void SpecializedDescriptorPool::OnReturnOne(VkDescriptorSet set) 
{
    assert(availableInstances < maxInstances);

    vkFreeDescriptorSets(pool.context->device(), pool.vkPool, 1, &set);

    availableInstances++;
}

DescriptorSet SpecializedDescriptorPool::Allocate() {
    assert(!empty());
    
    VkDescriptorSet set;

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool.vkPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;
    VK(vkAllocateDescriptorSets(pool.context->device(), &allocInfo, &set));
    availableInstances--;

    return DescriptorSet(set, this, pool.context);
}

bool SpecializedDescriptorPool::empty() {
    return availableInstances == 0;
}

SpecializedDescriptorPool::SpecializedDescriptorPool(
    RenderContext* context, 
    uint32_t countInstances,
    VkDescriptorSetLayout l,
    uint32_t countDesciptors, 
    const VkDescriptorPoolSize* sizes
): 
pool(context, countDesciptors, sizes, true), 
availableInstances(countInstances),
maxInstances(countInstances),
layout(l)
{}

DescriptorSet::~DescriptorSet() {
    if (pool == nullptr)
        return;

    pool->OnReturnOne(vkSet);
}

Descriptors::Descriptors(RenderContext& c): 
    FeatureSet(c),
    _layouts(), 
    _descriptorPools(), 
    _descriptorSetPool(),
    _preallocatedDescriptorSets(),
    frameId(0)
{
    _identityCache = std::make_shared<DescriptorSetIdentity>();
}

void Descriptors::OnMessage(DestroyMsg*) {
    for (auto p : _layouts) {
        vkDestroyDescriptorSetLayout(context.device(), p.second, nullptr); 
    }
}

void Descriptors::OnMessage(EarlyDestroyMsg*) {

    for (auto& pair : _preallocatedDescriptorSets) {
        pair.second.Forget();
    }
    _preallocatedDescriptorSets.clear();

    for (auto& pair : _descriptorSetPool) {
        pair.second.clear();
    }
    _descriptorSetPool.clear();
}

void Descriptors::OnMessage(BeginFrameMsg* m) {

    frameId = m->inFlightFrame;

    for (auto& pair : _descriptorSetPool) {
        pair.second.SetFrame(m->inFlightFrame);
    }
}

Ref<SpecializedDescriptorPool> Descriptors::CreateDescriptorPool(
    uint32_t countInstances,
    uint32_t countDescriptors,
    VkDescriptorSetLayout layout,
    MemChunk<VkDescriptorPoolSize> sizes
) {
    return context.New<SpecializedDescriptorPool>(countInstances, layout, countDescriptors, sizes.data);
}