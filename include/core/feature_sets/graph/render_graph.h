#pragma once
#include <common.h>
#include <feature_set.h>
#include <render_node.h>
#include <allocator_feature.h>

struct NodeWrapper {
    MemChunk<NodeDependency> inputDependency;
    MemChunk<NodeDependency> outpnputDependency;
    RenderNode* node;
};

struct API RenderGraph: FeatureSet,
    CanHandle<BeginFrameMsg>,
    CanHandle<DestroyMsg>
{
    using FeatureSet::FeatureSet;

    void AddNode(RenderNode&);

    void Render();

    void OnMessage(BeginFrameMsg*);
    void OnMessage(DestroyMsg*);

private:
    std::vector<NodeWrapper> nodes;
};