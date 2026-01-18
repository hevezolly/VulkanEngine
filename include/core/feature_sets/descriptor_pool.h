#pragma once
#include <feature_set.h>
#include <unordered_map>
#include <allocator_feature.h>
#include <handles.h>
#include <object_pool.h>
#include <resource_id.h>

struct DescriptorPool 
{
    VkDescriptorPool vkPool;
    DescriptorPool(RenderContext*, uint32_t count, const VkDescriptorPoolSize* sizes, bool createFree);
    DescriptorPool(RenderContext*, std::initializer_list<VkDescriptorPoolSize> sizes, bool createFree=false);

    RULE_NO(DescriptorPool)

    friend struct SpecializedDescriptorPool;
private:
    RenderContext* context;
};

void API __deallocate(RenderContext*, RawMemChunk memory, bool force=false);
VkDevice API __device(RenderContext*);
Allocator& API __get_alloc(RenderContext*);

struct API DescriptorSet {
    VkDescriptorSet vkSet;

    DescriptorSet(VkDescriptorSet set, SpecializedDescriptorPool* p, RenderContext* ctx): 
        vkSet(set), pool(p), context(ctx), currentIds(MemChunk<ResourceId>::Null()) {}

    template<typename T>
    void Update(T& value) {

        if (currentIds.size != T::size_resources()) {
            __deallocate(context, currentIds);
            currentIds = __get_alloc(context).HeapAllocate<ResourceId>(T::size_resources());
        }

        if (!value.FillUsedResources(currentIds.data)) {

            MemChunk<VkWriteDescriptorSet> writes = value.CollectDescriptorWrites(*context, vkSet);
    
            vkUpdateDescriptorSets(__device(context), writes.size, writes.data, 0, nullptr);
    
            __deallocate(context, writes, true);
        }
    }

    RULE_5(DescriptorSet)

private: 
    SpecializedDescriptorPool* pool;
    RenderContext* context;
    MemChunk<ResourceId> currentIds;
};

struct API SpecializedDescriptorPool 
{
    SpecializedDescriptorPool(
        RenderContext*, 
        uint32_t countInstances,
        VkDescriptorSetLayout layout,
        uint32_t countDesciptors, 
        const VkDescriptorPoolSize* sizes
    );

    RULE_NO(SpecializedDescriptorPool)

    bool empty();
    DescriptorSet Allocate();
    
    friend DescriptorSet;
private:
    void OnReturnOne(VkDescriptorSet set);
    VkDescriptorSetLayout layout;
    DescriptorPool pool;
    uint32_t availableInstances;
    uint32_t maxInstances;
};

struct API Descriptors : FeatureSet,
    CanHandle<DestroyMsg>,
    CanHandle<EarlyDestroyMsg>,
    CanHandle<BeginFrameMsg>
{
    using FeatureSet::FeatureSet;

    template<typename T>
    void Preallocate(uint32_t countInstances = 1) {
        TypeId id = getTypeId<T>();

        assert(countInstances >= 1);

        uint32_t countDescriptors = 0;
        MemChunk<VkDescriptorPoolSize> sizes = T::FillDescriptorSizes(context, countDescriptors);
        
        for (int i = 0; i < countDescriptors; i++) {
            sizes[i].descriptorCount *= countInstances;
        }

        if (_descriptorPools.find(id) == _descriptorPools.end())
            _descriptorPools[id] = std::vector<Ref<SpecializedDescriptorPool>>{};

        Ref<SpecializedDescriptorPool> pool = CreateDescriptorPool(
            countInstances, countDescriptors, GetLayout<T>(), sizes);

        _descriptorPools[id].push_back(pool);

        __deallocate(&context, sizes, true);
    }

    template<typename T>
    VkDescriptorSetLayout GetLayout() {
        TypeId id = getTypeId<T>();

        if (_layouts.find(id) == _layouts.end()) {
            
            TypeId id = getTypeId<T>();

            _layouts[id] = VK_NULL_HANDLE;

            MemChunk<VkDescriptorSetLayoutBinding> bindings = T::FillLayoutBindings(context);

            VkDescriptorSetLayoutCreateInfo createInfo {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
            createInfo.bindingCount = bindings.size;
            createInfo.flags = 0;
            createInfo.pBindings = bindings.data;

            vkCreateDescriptorSetLayout(__device(&context), &createInfo, nullptr, &_layouts[id]);
            __deallocate(&context, bindings, true);
        }

        return _layouts[id];
    }

    template<typename T>
    DescriptorSet CreateDescriptorSet() 
    {
        TypeId id = getTypeId<T>();

        Ref<SpecializedDescriptorPool> selectedPool = Ref<SpecializedDescriptorPool>::Null();

        if (_descriptorPools.find(id) != _descriptorPools.end()) {
            for (int i = 0; i < _descriptorPools[id].size(); i++) {
                if (!_descriptorPools[id][i]->empty())
                    selectedPool = _descriptorPools[id][i];
            }
        }

        if (selectedPool.isNull()) {
            Preallocate<T>();
            selectedPool = _descriptorPools[id].back();
        }

        return selectedPool->Allocate();
    }

    template<typename T>
    DescriptorSet& BorrowDescriptorSet(T& values) 
    {
        if (_allocatedDescriptors.size() < frameId + 1)
            _allocatedDescriptors.resize(frameId + 1);

        TypeId id = getTypeId<T>();

        if (!_allocatedDescriptors[frameId])
            _allocatedDescriptors[frameId] = std::make_unique<std::unordered_map<TypeId, ObjectPool<DescriptorSet>>>();

        ObjectPool<DescriptorSet>& pool = (*_allocatedDescriptors[frameId])[id];

        if (pool.isEmpty())
            pool.Insert(CreateDescriptorSet<T>());

        DescriptorSet& set = pool.BorrowAndForget();
        set.Update<T>(values);
        return set;
    }

    virtual void OnMessage(DestroyMsg*);
    virtual void OnMessage(BeginFrameMsg*);
    virtual void OnMessage(EarlyDestroyMsg*);

private:
    Ref<SpecializedDescriptorPool> CreateDescriptorPool(
        uint32_t, uint32_t, VkDescriptorSetLayout, MemChunk<VkDescriptorPoolSize> sizes);
    std::unordered_map<TypeId, VkDescriptorSetLayout> _layouts;
    std::unordered_map<TypeId, std::vector<Ref<SpecializedDescriptorPool>>> _descriptorPools;
    
    std::vector<std::unique_ptr<std::unordered_map<TypeId, ObjectPool<DescriptorSet>>>> _allocatedDescriptors;

    uint32_t frameId;
};