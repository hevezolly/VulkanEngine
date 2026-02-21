#pragma once
#include <common.h>
#include <feature_set.h>
#include <render_node.h>
#include <allocator_feature.h>
#include <unordered_map>
#include <unordered_set>
#include <render_context.h>
#include <synchronization.h>

struct NodeWrapper {
    QueueType queue;
    MemChunk<NodeDependency> inputDependency;
    MemChunk<uint32_t> inputVersions;
    MemChunk<NodeDependency> outpnputDependency;
    MemChunk<uint32_t> outputVersions;
    RenderNode* node;
};

struct VersionedResource {
    ResourceId id;
    uint32_t version;

    bool operator==(const VersionedResource& other) const {
        return id == other.id && version == other.version;
    }
};

namespace std {
    template <>
    struct hash<VersionedResource> {
        std::size_t operator()(const VersionedResource& pn) const noexcept {
            std::size_t h1 = std::hash<ResourceId>{}(pn.id);
            std::size_t h2 = std::hash<uint32_t>{}(pn.version);
            return h1 ^ (h2 << 1);
        }
    };
}

struct QueueTimelineValue {
    uint64_t timelineValue;
    uint32_t queueIndex;

    bool operator==(const QueueTimelineValue& other) const {
        return queueIndex == other.queueIndex && timelineValue == other.timelineValue;
    }
};

namespace std {
    template <>
    struct hash<QueueTimelineValue> {
        std::size_t operator()(const QueueTimelineValue& pn) const noexcept {
            std::size_t h1 = std::hash<uint32_t>{}(pn.queueIndex);
            std::size_t h2 = std::hash<uint64_t>{}(pn.timelineValue);
            return h1 ^ (h2 << 1);
        }
    };
}

struct API RenderGraph: FeatureSet,
    CanHandle<BeginFrameMsg>
{
    using FeatureSet::FeatureSet;

    template<typename T, typename... Args>
    T& AddNode(Args&&... args) {
        Allocator& alloc = context.Get<Allocator>();
        MemChunk<T> nodeData = alloc.BumpAllocate<T>();
        T* node = new (nodeData.data) T(context, std::forward<Args>(args)...);

        nodes.push_back(NodeWrapper {
            node->getTargetQueue(),
            MemChunk<NodeDependency>::Null(),
            MemChunk<uint32_t>::Null(),
            MemChunk<NodeDependency>::Null(),
            MemChunk<uint32_t>::Null(),
            node
        });

        return *node;
    }

    virtual void OnMessage(BeginFrameMsg*);
    
    void Run();
private:

    std::vector<NodeWrapper> nodes;

    FramedStorage<std::vector<Ref<Semaphore>>> semaphoresPerQueue;
    FramedStorage<std::vector<uint64_t>> semaphoreValuesPerQueue;
};