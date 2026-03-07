#pragma once
#include <feature_set.h>
#include <unordered_map>
#include <allocator_feature.h>
#include <handles.h>
#include <object_pool.h>
#include <resource_id.h>
#include <descriptor_identity.h>
#include <framed_object_pool.h>
#include <helper_functions.h>

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

struct API ShaderInputInstance {
    DescriptorSet* descriptor;
    uint32_t dynamicOffsetsCount;
    uint32_t* pDynamicOffsets;
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

        auto _ = Helpers::allocator(&context).BeginContext();

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
    }

    template<typename T>
    VkDescriptorSetLayout GetLayout() {
        TypeId id = getTypeId<T>();

        if (_layouts.find(id) == _layouts.end()) {
            
            auto _ = Helpers::allocator(&context).BeginContext();
            TypeId id = getTypeId<T>();

            _layouts[id] = VK_NULL_HANDLE;

            MemChunk<VkDescriptorSetLayoutBinding> bindings = T::FillLayoutBindings(context);

            VkDescriptorSetLayoutCreateInfo createInfo {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
            createInfo.bindingCount = bindings.size;
            createInfo.flags = 0;
            createInfo.pBindings = bindings.data;

            vkCreateDescriptorSetLayout(Helpers::device(&context), &createInfo, nullptr, &_layouts[id]);
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
    ShaderInputInstance BorrowDescriptorSet(const T& values) 
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

        auto result = _preallocatedDescriptorSets.emplace(set->identity, std::move(set));
        
        return UpdateDescriptorSetPreloaded(result->second, values);
    }

    template<typename T>
    ShaderInputInstance UpdateDescriptorSet(DescriptorSet& set, const T& values) {
        _identityCache->clear();
        values.FillDescriptorSetIdentity(*_identityCache);
        return UpdateDescriptorSetPreloaded(set, values);
    }

    virtual void OnMessage(DestroyMsg*);
    virtual void OnMessage(BeginFrameMsg*);
    virtual void OnMessage(EarlyDestroyMsg*);

private:

    template <typename T>
    ShaderInputInstance UpdateDescriptorSetPreloaded(DescriptorSet& set, const T& values) {
        Allocator& alloc = Helpers::allocator(&context);

        MemChunk<uint32_t> dynamicStates = alloc.BumpAllocate<uint32_t>(T::size_dynamic_states());

        auto _ = alloc.BeginContext();
        MemChunk<VkWriteDescriptorSet> writes = values.CollectDescriptorWrites(*context, set->vkSet, dynamicStates.data);
        MemBuffer<VkWriteDescriptorSet> actualWrites = alloc.BumpAllocate<VkWriteDescriptorSet>(writes.size);

        if (set.identity == nullptr) {
            vkUpdateDescriptorSets(Helpers::device(context), writes.size, writes.data, 0, nullptr);
            set.identity = std::make_shared(*_identityCache);
            return;
        }

        assert(set.identity->identities.size() == _identityCache->identities.size());
        assert(set.identity->identities.size() == writes.size);
        for (int i = 0; i < set.identity->identities.size(); i++) {
            if (set.identity->identities[i] == _identityCache->identities[i])
                continue;

            actualWrites.push_back(writes[i]);
        }
        
        if (actualWrites.size() > 0) {
            vkUpdateDescriptorSets(Helpers::device(context), actualWrites.size(), actualWrites.data(), 0, nullptr);
            set.identity = std::make_shared(*_identityCache);
        }

        return ShaderInputInstance {
            &set,
            dynamicStates.size,
            dynamicStates.data
        };
    }

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