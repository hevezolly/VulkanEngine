#include <render_graph.h>
#include <render_context.h>
#include <allocator_feature.h>
#include <command_pool.h>
#include <algorithm>

static constexpr VkPipelineStageFlags2 kStageOrder[] = {
    VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT,
    VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
    VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT,

    VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
    VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT,
    VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT,
    VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT,
    VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT,
    VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT,
    VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,

    VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
    VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
    VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,

    VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,

    VK_PIPELINE_STAGE_2_TRANSFER_BIT,

    VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
    VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,

    VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
};

inline VkPipelineStageFlags2 earliest_stage(VkPipelineStageFlags2 mask)
{
    for (VkPipelineStageFlags2 s : kStageOrder)
        if (mask & s)
            return s;

    return VK_PIPELINE_STAGE_2_NONE;
}

inline VkPipelineStageFlags2 min_stage(
    VkPipelineStageFlags2 a,
    VkPipelineStageFlags2 b)
{
    if (a == VK_PIPELINE_STAGE_2_NONE) return b;
    if (b == VK_PIPELINE_STAGE_2_NONE) return a;

    VkPipelineStageFlags2 ea = earliest_stage(a);
    VkPipelineStageFlags2 eb = earliest_stage(b);

    // find which appears first in the canonical order
    for (VkPipelineStageFlags2 s : kStageOrder)
    {
        if (s == ea) return ea;
        if (s == eb) return eb;
    }

    // fallback (should never happen)
    return ea;
}

struct ResourceUsage {
    VersionedResource resource;
    uint32_t node;
    uint32_t dependencyIndex;
};

struct SignalDescription {
    VkPipelineStageFlags2 stage;
    uint32_t syncContext;
};

struct QueueTimeStep {

    uint32_t node;
    std::vector<SignalDescription> signals;

    enum struct Type {
        Node,
        Wait,
        Signal
    } type;

    static QueueTimeStep Node(uint32_t node) {
        QueueTimeStep step;
        step.type = Type::Node;
        step.node = node;
        return step;
    }

    static QueueTimeStep Signal(SignalDescription signal) {
        QueueTimeStep step;
        step.type = Type::Signal;
        step.signals.push_back(signal);
        return step;
    }

    static QueueTimeStep Wait(std::vector<SignalDescription>&& wait) {
        QueueTimeStep step;
        step.type = Type::Wait;
        step.signals = std::move(wait);
        return step;
    }
};

struct SynchronizationContext {
    uint32_t queue;
    uint32_t timelineValue;
    VkPipelineStageFlags2 signalStage;
    bool explicitSemaphore;
    uint32_t waits;
};

struct GraphInstance {
    std::vector<NodeWrapper>& nodes;
    Allocator& alloc;
    std::unordered_map<VersionedResource, ResourceUsage> writes;
    std::unordered_map<VersionedResource, std::vector<ResourceUsage>> reads;

    std::vector<std::vector<ResourceUsage>> inEdges;
    std::vector<std::vector<VersionedResource>> outEdges;

    std::unordered_map<ResourceId, uint32_t> versions;
    std::vector<std::list<QueueTimeStep>> queueTimelines; 
    std::vector<uint32_t> sortedNodes;

    std::vector<SynchronizationContext> syncContexts;

    const std::vector<ResourceUsage>& getReads(VersionedResource r) {
        static const std::vector<ResourceUsage> emptyVector; 
        auto it = reads.find(r);
        if (it == reads.end())
            return emptyVector;
        return it->second;
    }

    void populateNode(NodeWrapper& wrapper, uint32_t nodeIndex) {
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

        outEdges.push_back(std::vector<VersionedResource>());
        inEdges.push_back(std::vector<ResourceUsage>());

        for (uint32_t i = 0; i < wrapper.inputDependency.size; i++) {

            ResourceId resource = wrapper.inputDependency[i].resource;
            VersionedResource r {resource, versions[resource]};
            wrapper.inputVersions[i] = r.version;
            
            inEdges[nodeIndex].push_back({r, nodeIndex, i});
            reads[r].push_back({r, nodeIndex, i});
        }

        for (uint32_t i = 0; i < wrapper.outpnputDependency.size; i++) {
            uint32_t writeVersion = ++versions[wrapper.outpnputDependency[i].resource];
            wrapper.outputVersions[i] = writeVersion;
            VersionedResource r {wrapper.outpnputDependency[i].resource, writeVersion};
            
            outEdges[nodeIndex].push_back(r);
            writes[r] = {r, nodeIndex, i};
        }
    }

    void findStartingNodes(std::vector<std::vector<VersionedResource>>& deps, MemBuffer<uint32_t>& data) {
        for (uint32_t i = 0; i < deps.size(); i++) {
            if (deps[i].size() == 0) {
                data.push_back(i);
            }
        }
    }

    void sortNodes() 
    {
        uint32_t capacity = outEdges.size();

        auto _ = alloc.BeginContext();

        MemBuffer<uint32_t> initialNodes = alloc.BumpAllocate<uint32_t>(capacity);
        findStartingNodes(outEdges, initialNodes);


        std::vector<std::vector<uint32_t>> dependencies;
        std::vector<std::unordered_set<uint32_t>> incomingEdges;

        for (uint32_t i = 0; i < outEdges.size(); i++) {
            dependencies.push_back(std::vector<uint32_t>());
            for (VersionedResource write : outEdges[i]) {
                const std::vector<ResourceUsage>& reads = getReads(write);

                for (ResourceUsage read: reads) {
                    if (read.node >= incomingEdges.size())
                        incomingEdges.resize(read.node + 1);
                    auto result = incomingEdges[read.node].insert(i);
                    
                    if (result.second) {
                        dependencies[i].push_back(read.node);
                    }
                }
            }
        }

        while (initialNodes.size() > 0) {
            uint32_t node = initialNodes.back();
            initialNodes.pop_back();
            sortedNodes.push_back(node);

            for (int m_index = dependencies[node].size() - 1; m_index >= 0; m_index--) {
                uint32_t m = dependencies[node][m_index];
                dependencies[node].pop_back();
                incomingEdges[m].erase(node);

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
    }

    void defineSyncronizationContexts() {

        uint32_t queuesCount = (uint32_t)QueueType::None;
        MemChunk<uint32_t> semaphoreValues = alloc.BumpAllocate<uint32_t>(queuesCount); 

        for (int i = 0; i < queuesCount; i++) {
            semaphoreValues[i] = 0;
        }

        syncContexts.resize(sortedNodes.size());

        for (uint32_t node: sortedNodes) {
            uint32_t queueIndex = (uint32_t)nodes[node].queue;

            VkPipelineStageFlags2 stage = 0;
            for (VersionedResource outResource: outEdges[node]) {
                ResourceUsage write = writes[outResource];
                bool found = false;

                for (ResourceUsage read : getReads(outResource)) {
                    uint32_t newQueue = (uint32_t)nodes[read.node].queue;

                    if (newQueue != queueIndex) {
                        found = true;
                        break;
                    }
                }

                if (found) {
                    stage |= nodes[write.node].outpnputDependency[write.dependencyIndex].state.accessStage;
                }
            }

            if (stage != 0) {
                syncContexts[node].timelineValue = ++semaphoreValues[queueIndex]; 
                syncContexts[node].queue = queueIndex;
                syncContexts[node].signalStage = stage;
            }
        }
    }

    void AssignWait(SignalDescription& signal, SignalDescription newSignal, bool requirebinary) {

        SynchronizationContext* oldContext;
        SynchronizationContext* newContext = &syncContexts[newSignal.syncContext];
        
        if (signal.stage != 0) {
            oldContext = &syncContexts[signal.syncContext];
        }

        if (signal.stage == 0 || oldContext->timelineValue < newContext->timelineValue) {
            if (signal.stage != 0) {
                assert(oldContext->waits > 0);
                oldContext->waits--;
            }

            newContext->waits++;
            signal.syncContext = newSignal.syncContext;
        }

        signal.stage |= newSignal.stage;
        syncContexts[signal.syncContext].explicitSemaphore |= requirebinary;
    }

    void buildTimelines() {
        uint32_t queuesCount = (uint32_t)QueueType::None;

        for (int i = 0; i < queuesCount; i++) {
            queueTimelines.push_back(std::list<QueueTimeStep>());
        }

        MemChunk<SignalDescription> perQueueSignals = alloc.BumpAllocate<SignalDescription>((uint32_t)QueueType::None);

        for (uint32_t node : sortedNodes) {
            uint32_t queueIndex = (uint32_t)nodes[node].queue;

            perQueueSignals.clearToZero();

            for (ResourceUsage inResource : inEdges[node]) {
                auto it = writes.find(inResource.resource);
                if (it == writes.end())
                    continue;

                ResourceUsage write = it->second;

                uint32_t newQueue = (uint32_t)nodes[write.node].queue; 
                if (newQueue == queueIndex) {
                    continue;
                }

                SignalDescription newSignal {
                    nodes[inResource.node].inputDependency[inResource.dependencyIndex].state.accessStage,
                    write.node
                };

                AssignWait(perQueueSignals[newQueue], newSignal, nodes[node].node->requireBinarySemaphore());
            }

            std::vector<SignalDescription> usedSignals;
            for (int i = 0; i < queuesCount; i++) {
                if (perQueueSignals[i].stage != 0)
                    usedSignals.push_back(perQueueSignals[i]);
            }

            if (usedSignals.size() > 0)
                queueTimelines[queueIndex].push_back(QueueTimeStep::Wait(std::move(usedSignals)));

            queueTimelines[queueIndex].push_back(QueueTimeStep::Node(node));

            if (syncContexts[node].timelineValue > 0)
                queueTimelines[queueIndex].push_back(QueueTimeStep::Signal(SignalDescription {
                    syncContexts[node].signalStage,
                    node
                }));
        }
    }

    void simplifyTimelines() {
        MemChunk<uint32_t> perQueueSignals = alloc.BumpAllocate<uint32_t>((uint32_t)QueueType::None);
        
        for (uint32_t queue = 0; queue < (uint32_t)QueueType::None; queue++) {
            perQueueSignals.clearToZero();
            auto it = queueTimelines[queue].begin();
            while (it != queueTimelines[queue].end()) {
                if (it->type == QueueTimeStep::Type::Wait) {
                    
                    for (int i = it->signals.size() - 1; i >= 0; --i) {
                        
                        SignalDescription signal = it->signals[i];

                        SynchronizationContext& context = syncContexts[signal.syncContext];

                        if (perQueueSignals[context.queue] > context.timelineValue) {
                            it->signals.erase(it->signals.begin() + i);
                        }
                        else {
                            perQueueSignals[context.queue] = context.timelineValue;
                        }
                    }
                } else if (it->type == QueueTimeStep::Type::Signal) {
                     for (int i = it->signals.size() - 1; i >= 0; --i) {
                        
                        SignalDescription signal = it->signals[i];

                        SynchronizationContext& context = syncContexts[signal.syncContext];

                        if (context.waits == 0) {
                            it->signals.erase(it->signals.begin() + i);
                        }
                    }
                }

                if (it->type != QueueTimeStep::Type::Node && it->signals.size() == 0) {
                    it = queueTimelines[queue].erase(it);
                    continue;
                }

                ++it;
            }
        }

    }
};

void RenderGraph::BuildGraph(TransferCommandBuffer& commandBuffer) {

    if (nodes.size() == 0)
        return;

    Allocator& alloc = context.Get<Allocator>();
    
    auto _ = alloc.BeginContext();

    GraphInstance instance {
        nodes,
        alloc
    };

    uint32_t index = 0;
    for (NodeWrapper& node : nodes) {
        instance.populateNode(node, index++);
    }

    instance.sortNodes();
    instance.defineSyncronizationContexts();
    instance.buildTimelines();
    instance.simplifyTimelines();

    for (uint32_t nodeId : instance.sortedNodes) {
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