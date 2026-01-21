#pragma once
#include <common.h>
#include <feature_set.h>
#include <render_node.h>
#include <allocator_feature.h>
#include <unordered_map>
#include <unordered_set>

struct NodeWrapper {
    MemChunk<NodeDependency> inputDependency;
    MemChunk<uint32_t> inputVersions;
    MemChunk<NodeDependency> outpnputDependency;
    MemChunk<uint32_t> outputVersions;
    RenderNode* node;
};

struct ResourceUsage {
    uint32_t version;
    uint32_t nodeIndex;
};

struct API RenderGraph: FeatureSet,
    CanHandle<BeginFrameMsg>,
    CanHandle<DestroyMsg>
{
    using FeatureSet::FeatureSet;

    void AddNode(RenderNode&);

    void BuildGraph();

    void OnMessage(BeginFrameMsg*);
    void OnMessage(DestroyMsg*);

private:

    MemChunk<uint32_t> sortNodes();

    std::vector<NodeWrapper> nodes;
    std::unordered_map<ResourceId, uint32_t> _versions;

    std::unordered_map<ResourceId, ResourceUsage> _lastWrite;
    std::vector<std::vector<uint32_t>> _dependencies;
    std::vector<std::unordered_set<uint32_t>> _incomingEdges;

};