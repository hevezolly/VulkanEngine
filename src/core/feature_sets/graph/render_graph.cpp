#include <render_graph.h>
#include <render_context.h>
#include <allocator_feature.h>

void RenderGraph::AddNode(RenderNode& node) {

    uint32_t nodeIndex = nodes.size();

    NodeWrapper wrapper;
    wrapper.inputDependency = node.getInputDependencies();
    wrapper.outpnputDependency = node.getOutputDependencies();
    wrapper.node = &node;
    wrapper.inputVersions = context.Get<Allocator>().BumpAllocate<uint32_t>(wrapper.inputDependency.size);
    wrapper.outputVersions = context.Get<Allocator>().BumpAllocate<uint32_t>(wrapper.outputVersions.size);

    
    _dependencies.push_back(std::vector<uint32_t>());
    _incomingEdges.push_back(std::unordered_set<uint32_t>());

    for (int i = 0; i < wrapper.inputDependency.size; i++) {

        ResourceId resource = wrapper.inputDependency[i].resource;

        wrapper.inputVersions[i] = _versions[resource];
        
        if (_lastWrite.find(resource) != _lastWrite.end()) {
            assert(_lastWrite[resource].version == _versions[resource]);
            _dependencies[_lastWrite[resource].nodeIndex].push_back(nodeIndex);
            _incomingEdges[nodeIndex].insert(_lastWrite[resource].nodeIndex);
        }
    }

    for (int i = 0; i < wrapper.outpnputDependency.size; i++) {
        uint32_t writeVersion = ++_versions[wrapper.outpnputDependency[i].resource];
        wrapper.outputVersions[i] = writeVersion;
        ResourceUsage& usage = _lastWrite[wrapper.outpnputDependency[i].resource];
        usage.version = writeVersion;
        usage.nodeIndex = nodeIndex;
    }

    nodes.push_back(wrapper);
}

uint32_t findStartingNodes(std::vector<std::vector<uint32_t>>& deps, uint32_t* data) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < deps.size(); i++) {
        if (deps[i].size() == 0) {
            data[count++] = i;
        }
    }

    return count;
}

MemChunk<uint32_t> RenderGraph::sortNodes() {
    Allocator& alloc = context.Get<Allocator>();
    uint32_t capacity = _dependencies.size();
    MemChunk<uint32_t> finalSortedNodes = alloc.BumpAllocate<uint32_t>(capacity);
    uint32_t finalSortedIndex = 0;
    MemChunk<uint32_t> initialNodes = alloc.BumpAllocate<uint32_t>(capacity);
    uint32_t count = findStartingNodes(_dependencies, initialNodes.data);

    while (count > 0) {
        uint32_t node = initialNodes[--count];
        finalSortedNodes[finalSortedIndex++] = node;

        for (int m_index = _dependencies[node].size() - 1; m_index >= 0; m_index--) {
            uint32_t m = _dependencies[node][m_index];
            _dependencies.pop_back();
            _incomingEdges[m].erase(m);

            if (_incomingEdges[m].size() == 0) {
                assert(count <= capacity-1);
                initialNodes[count++] = m;
            }
        }
    }

    for (int i = 0; i < capacity; i++) {
        assert(_dependencies[i].size() == 0);
        assert(_incomingEdges[i].size() == 0);
    }

    alloc.Free(initialNodes);

    return finalSortedNodes;
}

void RenderGraph::BuildGraph() {

    MemChunk<uint32_t> sortedNodes = sortNodes();

    
}