#pragma once
#include <feature_set.h>
#include <unordered_map>
#include <allocator_feature.h>
#include <handles.h>

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

struct SpecializedDescriptorSet {
    VkDescriptorSet vkSet;

    SpecializedDescriptorSet(VkDescriptorSet set, SpecializedDescriptorPool* p): 
        vkSet(set), pool(p) {}

    RULE_5(SpecializedDescriptorSet)

private: 
    SpecializedDescriptorPool* pool;
};

struct SpecializedDescriptorPool 
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
    SpecializedDescriptorSet Allocate();
    
    friend SpecializedDescriptorSet;
private:
    void OnReturnOne(VkDescriptorSet set);
    VkDescriptorSetLayout layout;
    DescriptorPool pool;
    uint32_t availableInstances;
    uint32_t maxInstances;
};

struct Descriptors : FeatureSet,
    CanHandle<DestroyMsg>,
    CanHandle<EarlyDestroyMsg>,
    CanHandle<BeginFrameMsg>
{
    using FeatureSet::FeatureSet;

    template<typename T>
    void Preallocate(uint32_t countInstances = 1) {
        std::type_index id = getTypeId<T>();

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

        Deallocate(sizes, true);
    }

    template<typename T>
    VkDescriptorSetLayout GetLayout() {
        std::type_index id = getTypeId<T>();

        if (_layouts.find(id) == _layouts.end()) {
            
            std::type_index id = getTypeId<T>();

            _layouts[id] = VK_NULL_HANDLE;

            MemChunk<VkDescriptorSetLayoutBinding> bindings = T::FillLayoutBindings(context);

            VkDescriptorSetLayoutCreateInfo createInfo {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
            createInfo.bindingCount = bindings.size;
            createInfo.flags = 0;
            createInfo.pBindings = bindings.data;

            vkCreateDescriptorSetLayout(device(), &createInfo, nullptr, &_layouts[id]);
            Deallocate(bindings, true);
        }

        return _layouts[id];
    }

    template<typename T>

    VkDescriptorSet UpdateDescriptorSet(T& values) {
        std::type_index id = getTypeId<T>();

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

        if (allocatedSets.size() < frameId + 1)
            allocatedSets.resize(frameId + 1);

        allocatedSets[frameId].push_back(selectedPool->Allocate());

        VkDescriptorSet set = allocatedSets[frameId].back().vkSet;
        MemChunk<VkWriteDescriptorSet> writes = values.CollectDescriptorWrites(context, set);

        vkUpdateDescriptorSets(device(), writes.size, writes.data, 0, nullptr);

        Deallocate(writes, true);

        return set;
    }

    virtual void OnMessage(DestroyMsg*);
    virtual void OnMessage(BeginFrameMsg*);
    virtual void OnMessage(EarlyDestroyMsg*);

private:
    void Deallocate(RawMemChunk memory, bool force=false);
    VkDevice device();
    Ref<SpecializedDescriptorPool> CreateDescriptorPool(
        uint32_t, uint32_t, VkDescriptorSetLayout, MemChunk<VkDescriptorPoolSize> sizes);
    std::unordered_map<std::type_index, VkDescriptorSetLayout> _layouts;
    std::unordered_map<std::type_index, std::vector<Ref<SpecializedDescriptorPool>>> _descriptorPools;

    std::vector<std::vector<SpecializedDescriptorSet>> allocatedSets;
    uint32_t frameId;
};