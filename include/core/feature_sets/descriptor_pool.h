#pragma once
#include <feature_set.h>
#include <unordered_map>
#include <allocator_feature.h>
#include <handles.h>
#include <object_pool.h>
#include <resource_id.h>
#include <descriptor_identity.h>
#include <framed_object_pool.h>

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
    std::shared_ptr<DescriptorSetIdentity> identity;

    DescriptorSet(
        VkDescriptorSet set, 
        SpecializedDescriptorPool* p, 
        RenderContext* ctx): 
        vkSet(set), pool(p), context(ctx), identity(nullptr) {}

    

    RULE_5(DescriptorSet)

private: 
    SpecializedDescriptorPool* pool;
    RenderContext* context;
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
    Descriptors(RenderContext&);

    template<typename T>
    void Preallocate(uint32_t countInstances = 1) {
        TypeId id = getTypeId<T>();

        assert(countInstances >= 1);

        uint32_t countDescriptors = 0;
        MemChunk<VkDescriptorPoolSize> sizes = T::FillDescriptorSizes(context, countDescriptors);
        
        for (int i = 0; i < countDescriptors; i++) {
            sizes[i].descriptorCount *= countInstances;
        }

        _preallocatedPoolSize[id] += countInstances;

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
            Preallocate<T>(std::max(_preallocatedPoolSize[id], 1));
            selectedPool = _descriptorPools[id].back();
        }

        return selectedPool->Allocate();
    }

    template<typename T>
    DescriptorSet& BorrowDescriptorSet(T& values) 
    {
        _identityCache->clear();
        values.FillDescriptorSetIdentity(*_identityCache);

        auto existing = _preallocatedDescriptorSets.find(_identityCache);

        TypeId id = getTypeId<T>();

        auto it = _descriptorSetPool.find(id);
        if (it == _descriptorSetPool.end()) {
            _descriptorSetPool[id].SetFrame(frameId);
        }

        if (existing != _preallocatedDescriptorSets.end()) {
            existing->second.MoveTo(_descriptorSetPool[id].CurrentPool());
            return existing->second;
        }

        if (_descriptorSetPool[id].isEmpty()) {
            _descriptorSetPool[id].Insert(CreateDescriptorSet<T>())
        }

        Borrowed<DescriptorSet> set = _descriptorSetPool[id].Borrow();

        auto toReplace = _preallocatedDescriptorSets.find(set->identity);
        if (toReplace != _preallocatedDescriptorSets.end()) {
            toReplace->second.Forget();
            _preallocatedDescriptorSets.erase(toReplace);
        }

        MemChunk<VkWriteDescriptorSet> writes = values.CollectDescriptorWrites(*context, set->vkSet);
        vkUpdateDescriptorSets(__device(context), writes.size, writes.data, 0, nullptr);
        __deallocate(context, writes, true);

        set->identity = std::make_shared(*_identityCache);

        auto result = _preallocatedDescriptorSets.emplace(set->identity, std::move(set));

        return *(result->second);
    }

    virtual void OnMessage(DestroyMsg*);
    virtual void OnMessage(BeginFrameMsg*);
    virtual void OnMessage(EarlyDestroyMsg*);

private:

    struct HashDSIByValue {
        std::size_t operator()(const std::shared_ptr<DescriptorSetIdentity> ptr) const {
            if (ptr == nullptr) {
                return 0;
            }

            return std::hash<DescriptorSetIdentity>()(*ptr);
        }
    };

    struct EqualDSIByValue{
        bool operator()(const std::shared_ptr<DescriptorSetIdentity> ptr1, const std::shared_ptr<DescriptorSetIdentity> ptr2) const {
            if (ptr1 == nullptr && ptr2 == nullptr) return true;
            if (ptr1 == nullptr || ptr2 == nullptr) return false;

            return *ptr1 == *ptr2;
        }
    };

    Ref<SpecializedDescriptorPool> CreateDescriptorPool(
        uint32_t, uint32_t, VkDescriptorSetLayout, MemChunk<VkDescriptorPoolSize> sizes);
    std::unordered_map<TypeId, VkDescriptorSetLayout> _layouts;
    std::unordered_map<TypeId, std::vector<Ref<SpecializedDescriptorPool>>> _descriptorPools;
    std::unordered_map<TypeId, uint32_t> _preallocatedPoolSize;

    std::unordered_map<std::shared_ptr<DescriptorSetIdentity>, Borrowed<DescriptorSet>, HashDSIByValue, EqualDSIByValue> _preallocatedDescriptorSets;

    std::unordered_map<TypeId, FramedObjectPool<DescriptorSet>> _descriptorSetPool;

    std::shared_ptr<DescriptorSetIdentity> _identityCache;
    uint32_t frameId;
};