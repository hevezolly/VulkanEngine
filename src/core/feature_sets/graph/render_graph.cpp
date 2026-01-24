#include <render_graph.h>
#include <render_context.h>
#include <allocator_feature.h>
#include <command_pool.h>

struct ResourceUsage {
    uint32_t version;
    uint32_t nodeIndex;
};

void RenderGraph::AddNode(RenderNode& node) {

    uint32_t nodeIndex = nodes.size();

    NodeWrapper wrapper;
    wrapper.inputDependency = node.getInputDependencies();
    wrapper.outpnputDependency = node.getOutputDependencies();
    wrapper.node = &node;
    wrapper.inputVersions = context.Get<Allocator>().HeapAllocate<uint32_t>(wrapper.inputDependency.size);
    wrapper.outputVersions = context.Get<Allocator>().HeapAllocate<uint32_t>(wrapper.outputVersions.size);

    std::unordered_map<ResourceId, uint32_t> versions;
    std::unordered_map<ResourceId, ResourceUsage> lastWrite;
    
    _dependencies.push_back(std::vector<uint32_t>());
    _incomingEdges.push_back(std::vector<uint32_t>());

    for (int i = 0; i < wrapper.inputDependency.size; i++) {

        ResourceId resource = wrapper.inputDependency[i].resource;

        wrapper.inputVersions[i] = versions[resource];
        
        if (lastWrite.find(resource) != lastWrite.end()) {
            assert(lastWrite[resource].version == versions[resource]);
            _dependencies[lastWrite[resource].nodeIndex].push_back(nodeIndex);
            _incomingEdges[nodeIndex].push_back(lastWrite[resource].nodeIndex);
        }
    }

    for (int i = 0; i < wrapper.outpnputDependency.size; i++) {
        uint32_t writeVersion = ++versions[wrapper.outpnputDependency[i].resource];
        wrapper.outputVersions[i] = writeVersion;
        ResourceUsage& usage = lastWrite[wrapper.outpnputDependency[i].resource];
        usage.version = writeVersion;
        usage.nodeIndex = nodeIndex;
    }

    nodes.push_back(wrapper);
}

void findStartingNodes(std::vector<std::vector<uint32_t>>& deps, MemBuffer<uint32_t>& data) {
    for (uint32_t i = 0; i < deps.size(); i++) {
        if (deps[i].size() == 0) {
            data.push_back(i);
        }
    }
}

MemBuffer<uint32_t> RenderGraph::sortNodes() {
    Allocator& alloc = context.Get<Allocator>();
    uint32_t capacity = _dependencies.size();

    MemBuffer<uint32_t> finalSortedNodes = alloc.BumpAllocate<uint32_t>(capacity);

    auto _ = alloc.BeginContext();

    MemBuffer<uint32_t> initialNodes = alloc.BumpAllocate<uint32_t>(capacity);
    findStartingNodes(_dependencies, initialNodes);

    MemChunk<MemBuffer<uint32_t>> dependencies = alloc.BumpAllocate<MemBuffer<uint32_t>>(_dependencies.size());
    for (int i = 0; i < _dependencies.size(); i++) {
        dependencies[i] = alloc.BumpAllocate<uint32_t>(_dependencies[i].size());
        std::memcpy(dependencies[i].data(), _dependencies[i].data(), _dependencies[i].size() * sizeof(uint32_t));
    }

    MemChunk<MemBuffer<uint32_t>> incomingEdges = alloc.BumpAllocate<MemBuffer<uint32_t>>(_incomingEdges.size());
    for (int i = 0; i < _incomingEdges.size(); i++) {
        incomingEdges[i] = alloc.BumpAllocate<uint32_t>(_incomingEdges[i].size());
        std::memcpy(incomingEdges[i].data(), _incomingEdges[i].data(), _incomingEdges[i].size() * sizeof(uint32_t));
    }

    while (initialNodes.size() > 0) {
        uint32_t node = initialNodes.back();
        initialNodes.pop_back();
        finalSortedNodes.push_back(node);

        for (int m_index = dependencies[node].size() - 1; m_index >= 0; m_index--) {
            uint32_t m = dependencies[node][m_index];
            dependencies[node].pop_back();
            incomingEdges[m].fast_delete(m);

            if (incomingEdges[m].size() == 0) {
                assert(initialNodes.size() <= capacity-1);
                initialNodes.push_back(m);
            }
        }
    }

    for (int i = 0; i < capacity; i++) {
        assert(dependencies[i].size() == 0);
        assert(incomingEdges[i].size() == 0);
    }

    return finalSortedNodes;
}

void RenderGraph::BuildGraph(TransferCommandBuffer& commandBuffer) {
    Allocator& alloc = context.Get<Allocator>();
    
    auto _ = alloc.BeginContext();

    MemBuffer<uint32_t> sortedNodes = sortNodes();

    for (uint32_t nodeId : sortedNodes) {
        const NodeWrapper& node = nodes[nodeId];

        MemBuffer<ResourceId> resources = alloc.BumpAllocate<ResourceId>(
            node.inputDependency.size + node.outpnputDependency.size);
        MemBuffer<ResourceState> states = alloc.BumpAllocate<ResourceState>(
            node.inputDependency.size + node.outpnputDependency.size);
        for (int i = 0; i < node.inputDependency.size; i++) {
            
            if (node.inputDependency[i].resource.type() != ResourceType::Image &&
                node.inputDependency[i].resource.type() != ResourceType::Buffer)
                continue;

            resources.push_back(node.inputDependency[i].resource);
            states.push_back(node.inputDependency[i].state);
        }

        for (int i = 0; i < node.outpnputDependency.size; i++) {
            
            if (node.outpnputDependency[i].resource.type() != ResourceType::Image &&
                node.outpnputDependency[i].resource.type() != ResourceType::Buffer)
                continue;

            resources.push_back(node.outpnputDependency[i].resource);
            states.push_back(node.outpnputDependency[i].state);
        }

        commandBuffer.Barrier(resources.size(), resources.data(), states.data());
    
        node.node->Record(commandBuffer);
    }
}