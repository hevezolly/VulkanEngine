#include <render_graph.h>
#include <render_context.h>
#include <allocator_feature.h>
#include <command_pool.h>
#include <algorithm>

struct ResourceUsage {
    uint32_t version;
    uint32_t nodeIndex;
};

void populateNode(NodeWrapper& wrapper, uint32_t nodeIndex, Allocator& alloc, 
    std::vector<std::vector<uint32_t>>& outEdges,
    std::vector<std::vector<uint32_t>>& inEdges,
    std::unordered_map<ResourceId, uint32_t> versions,
    std::unordered_map<ResourceId, ResourceUsage> lastWrite
) {
    uint32_t inputSize = wrapper.node->getInputDependenciesCount();
    uint32_t outputSize = wrapper.node->getOutputDependenciesCount();
    wrapper.queue = wrapper.node->getTargetQueue();
    if (inputSize != 0) {
        wrapper.inputDependency = alloc.BumpAllocate<NodeDependency>(inputSize);
        wrapper.inputVersions = alloc.BumpAllocate<uint32_t>(inputSize);
        wrapper.node->getInputDependencies(wrapper.inputDependency.data);
    }

    if (outputSize != 0) {
        wrapper.outpnputDependency = alloc.BumpAllocate<NodeDependency>(outputSize);
        wrapper.outputVersions = alloc.BumpAllocate<uint32_t>(outputSize);
        wrapper.node->getOutputDependencies(wrapper.outpnputDependency.data);
    }

    outEdges.push_back(std::vector<uint32_t>());
    inEdges.push_back(std::vector<uint32_t>());

    for (int i = 0; i < wrapper.inputDependency.size; i++) {

        ResourceId resource = wrapper.inputDependency[i].resource;

        wrapper.inputVersions[i] = versions[resource];
        
        if (lastWrite.find(resource) != lastWrite.end()) {
            assert(lastWrite[resource].version == versions[resource]);
            uint32_t lastWriteIndex = lastWrite[resource].nodeIndex;
            if (std::find(outEdges[lastWriteIndex].begin(), 
                outEdges[lastWriteIndex].end(), nodeIndex) == 
                outEdges[lastWriteIndex].end())
                outEdges[lastWriteIndex].push_back(nodeIndex);
            
            if (std::find(inEdges[nodeIndex].begin(), 
                inEdges[nodeIndex].end(), lastWriteIndex) == 
                inEdges[nodeIndex].end())
                inEdges[nodeIndex].push_back(lastWriteIndex);
        }
    }

    for (int i = 0; i < wrapper.outpnputDependency.size; i++) {
        uint32_t writeVersion = ++versions[wrapper.outpnputDependency[i].resource];
        wrapper.outputVersions[i] = writeVersion;
        ResourceUsage& usage = lastWrite[wrapper.outpnputDependency[i].resource];
        usage.version = writeVersion;
        usage.nodeIndex = nodeIndex;
    }
}

void findStartingNodes(std::vector<std::vector<uint32_t>>& deps, MemBuffer<uint32_t>& data) {
    for (uint32_t i = 0; i < deps.size(); i++) {
        if (deps[i].size() == 0) {
            data.push_back(i);
        }
    }
}

MemBuffer<uint32_t> sortNodes(
    RenderContext& context,
    std::vector<std::vector<uint32_t>>& outEdges,
    std::vector<std::vector<uint32_t>>& inEdges
) 
{
    Allocator& alloc = context.Get<Allocator>();
    uint32_t capacity = outEdges.size();

    MemBuffer<uint32_t> finalSortedNodes = alloc.BumpAllocate<uint32_t>(capacity);

    auto _ = alloc.BeginContext();

    MemBuffer<uint32_t> initialNodes = alloc.BumpAllocate<uint32_t>(capacity);
    findStartingNodes(outEdges, initialNodes);

    MemChunk<MemBuffer<uint32_t>> dependencies = alloc.BumpAllocate<MemBuffer<uint32_t>>(outEdges.size());
    for (int i = 0; i < outEdges.size(); i++) {
        dependencies[i] = alloc.BumpAllocate<uint32_t>(outEdges[i].size());
        std::memcpy(dependencies[i].data(), outEdges[i].data(), outEdges[i].size() * sizeof(uint32_t));
    }

    MemChunk<MemBuffer<uint32_t>> incomingEdges = alloc.BumpAllocate<MemBuffer<uint32_t>>(inEdges.size());
    for (int i = 0; i < inEdges.size(); i++) {
        incomingEdges[i] = alloc.BumpAllocate<uint32_t>(inEdges[i].size());
        std::memcpy(incomingEdges[i].data(), inEdges[i].data(), inEdges[i].size() * sizeof(uint32_t));
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

struct QueueTimeStep {

    union {
        uint32_t node;
        uint32_t semaphoreValue;
    };

    enum Type {
        Node,
        Wait,
        Signal
    } type;
};

void RenderGraph::BuildGraph(TransferCommandBuffer& commandBuffer) {

    if (nodes.size() == 0)
        return;

    Allocator& alloc = context.Get<Allocator>();
    
    auto _ = alloc.BeginContext();

    std::unordered_map<ResourceId, uint32_t> versions;
    std::unordered_map<ResourceId, ResourceUsage> lastWrite;
    std::vector<std::vector<uint32_t>> outEdges;
    std::vector<std::vector<uint32_t>> inEdges;

    uint32_t index = 0;
    for (NodeWrapper& node : nodes) {
        populateNode(node, index++, alloc, outEdges, inEdges, versions, lastWrite);
    }

    // topologically sort nodes per queue
    MemBuffer<uint32_t> sortedNodes = sortNodes(context, outEdges, inEdges);
    uint32_t queuesCount = (uint32_t)QueueType::None;
    MemChunk<MemBuffer<uint32_t>> sortedNodesPerQueue = alloc.BumpAllocate<MemBuffer<uint32_t>>(queuesCount);
    MemChunk<MemBuffer<uint32_t>> submissionBoundaries = alloc.BumpAllocate<MemBuffer<uint32_t>>(queuesCount);
    MemChunk<uint32_t> semaphoreValues = alloc.BumpAllocate<uint32_t>(queuesCount); // TODO

    for (int i = 0; i < queuesCount; i++) {
        semaphoreValues[i] = 0;
        sortedNodesPerQueue[i] = alloc.BumpAllocate<uint32_t>(sortedNodes.size());
        submissionBoundaries[i] = alloc.BumpAllocate<uint32_t>(sortedNodes.size() - 1);

        for (uint32_t node : sortedNodes) {
            uint32_t queueIndex = (uint32_t)nodes[node].queue;

            assert(queueIndex < (uint32_t)QueueType::None);
            sortedNodesPerQueue[queueIndex].push_back(node);
        }
    }

    for (int queue = 0; queue < queuesCount; queue++) {

        QueueType currentQueue = (QueueType)queue;

        for (int i = 0; i < sortedNodesPerQueue[queue].size(); i++) {
            uint32_t node = sortedNodesPerQueue[queue][i];

            if (i > 0)
        }

        for (uint32_t node : sortedNodesPerQueue[queue]) {

            for (uint32_t dependency : inEdges[node]) {
                QueueType dependencyQueue = nodes[dependency].queue;
                if (dependencyQueue != currentQueue) {
                    submissionBoundaries[queue]
                }

            }

        }

    }



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

    nodes.clear();

}