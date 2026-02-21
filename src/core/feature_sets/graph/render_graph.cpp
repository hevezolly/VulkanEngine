#include <render_graph.h>
#include <render_context.h>
#include <allocator_feature.h>
#include <command_pool.h>
#include <algorithm>
#include <resources.h>
#include <array>

template<typename T>
struct PerQueueStorage {
    std::array<T, static_cast<size_t>(QueueType::None)> storage;

    T& operator[](uint32_t index) {
        return storage[index];
    }

    void assign_to_all(const T& value) {
        for (uint32_t i = 0; i < storage.size(); i++) {
            storage[i] = value;
        }
    }
    
    void clear_to_zero() {
        memset(storage.data(), 0, sizeof(T) * storage.size());
    }
};

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
    QueueTimelineValue timelineValue;
    VkPipelineStageFlags2 signalStage;
    bool forceBinary;
    std::optional<ResourceId> externalSyncSource;
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
    PerQueueStorage<std::list<QueueTimeStep>> queueTimelines;
    std::vector<uint32_t> sortedNodes;

    std::unordered_map<ResourceId, PerQueueStorage<uint32_t>> externalSync;
    std::unordered_map<VersionedResource, ResourceState> forceWriteStates;

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
            
            if (wrapper.inputDependency[i].stateByWriter) {
                forceWriteStates[r] = wrapper.inputDependency[i].state;
            }

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

    void findStartingNodes(uint32_t count, MemBuffer<uint32_t>& data) {
        for (uint32_t i = 0; i < count; i++) {
            bool found = false;
            for (ResourceUsage usage : inEdges[i]) {
                if (writes.find(usage.resource) != writes.end()) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                data.push_back(i);
            }
        }
    }

    void sortNodes() 
    {
        uint32_t capacity = outEdges.size();

        auto _ = alloc.BeginContext();

        MemBuffer<uint32_t> initialNodes = alloc.BumpAllocate<uint32_t>(capacity);
        findStartingNodes(capacity, initialNodes);

        std::vector<std::vector<uint32_t>> outcomingEdges;
        std::vector<std::unordered_set<uint32_t>> incomingEdges;

        for (uint32_t i = 0; i < outEdges.size(); i++) {
            outcomingEdges.push_back(std::vector<uint32_t>());
            for (VersionedResource write : outEdges[i]) {
                const std::vector<ResourceUsage>& reads = getReads(write);

                for (ResourceUsage read: reads) {
                    if (read.node >= incomingEdges.size())
                        incomingEdges.resize(read.node + 1);
                    auto result = incomingEdges[read.node].insert(i);
                    
                    if (result.second) {
                        outcomingEdges[i].push_back(read.node);
                    }
                }
            }
        }

        while (initialNodes.size() > 0) {
            uint32_t node = initialNodes.back();
            initialNodes.pop_back();
            sortedNodes.push_back(node);

            for (int m_index = outcomingEdges[node].size() - 1; m_index >= 0; m_index--) {
                uint32_t m = outcomingEdges[node][m_index];
                outcomingEdges[node].pop_back();
                incomingEdges[m].erase(node);

                if (incomingEdges[m].size() == 0) {
                    assert(initialNodes.size() <= capacity-1);
                    initialNodes.push_back(m);
                }
            }
        }

        for (int i = 0; i < capacity; i++) {
            assert(outcomingEdges[i].size() == 0);
            assert(incomingEdges[i].size() == 0);
        }
    }

    void tryAddResourceExternalSync(ResourceId id, RenderContext& context, uint32_t queue, uint32_t& timelineValue) {
        if (context.Get<Resources>().ResourceRequiresSynchronization(id)) {
            bool notFound = externalSync.find(id) == externalSync.end();
            auto& sync = externalSync[id];
            
            if (notFound) {
                sync.assign_to_all(UINT32_MAX);
            }

            if (sync[queue] == UINT32_MAX) {
                sync[queue] = syncContexts.size();
                syncContexts.push_back(
                    SynchronizationContext {
                        QueueTimelineValue{queue, ++timelineValue},
                        VK_PIPELINE_STAGE_2_NONE,
                        false,
                        id,
                        0
                    }
                ); 
            }
        }
    }

    void defineSyncronizationContexts(RenderContext& context) {

        uint32_t queuesCount = (uint32_t)QueueType::None;
        PerQueueStorage<uint32_t> semaphoreValues;

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

                tryAddResourceExternalSync(outResource.id, context, queueIndex, semaphoreValues[queueIndex]);

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

            for (ResourceUsage inResource: inEdges[node]) {
                tryAddResourceExternalSync(inResource.resource.id, context, queueIndex, semaphoreValues[queueIndex]);
            }
            
            if (stage != 0) {
                syncContexts[node].timelineValue.timelineValue = ++semaphoreValues[queueIndex]; 
                syncContexts[node].timelineValue.queueIndex = queueIndex;
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

        if (signal.stage == 0 || oldContext->timelineValue.timelineValue < newContext->timelineValue.timelineValue) {
            if (signal.stage != 0) {
                assert(oldContext->waits > 0);
                oldContext->waits--;
            }

            newContext->waits++;
            signal.syncContext = newSignal.syncContext;
        }

        signal.stage |= newSignal.stage;
        syncContexts[signal.syncContext].forceBinary |= requirebinary;
    }

    void buildTimelines() {
        uint32_t queuesCount = (uint32_t)QueueType::None;
        auto _ = alloc.BeginContext();
        MemChunk<SignalDescription> perQueueSignals = alloc.BumpAllocate<SignalDescription>((uint32_t)QueueType::None);

        for (uint32_t node : sortedNodes) {
            uint32_t queueIndex = (uint32_t)nodes[node].queue;

            perQueueSignals.clearToZero();

            std::vector<SignalDescription> usedSignals;

            for (ResourceUsage inResource : inEdges[node]) {
                auto it = writes.find(inResource.resource);
                if (it == writes.end())
                    continue;

                ResourceUsage write = it->second;

                auto extSync = externalSync.find(inResource.resource.id);
                if (extSync != externalSync.end()) {
                    assert(extSync->second[queueIndex] != UINT32_MAX);

                    usedSignals.push_back(SignalDescription {
                        nodes[inResource.node].inputDependency[inResource.dependencyIndex].state.accessStage,
                        extSync->second[queueIndex]
                    });
                }
                
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

            for (VersionedResource outResource: outEdges[node]) {
                auto extSync = externalSync.find(outResource.id);
                if (extSync != externalSync.end()) {
                    ResourceUsage write = writes[outResource];
                    assert(extSync->second[queueIndex] != UINT32_MAX);
                    uint32_t syncContext = extSync->second[queueIndex];
                    bool found = false;
                    for (auto alreadySignalled : usedSignals) {
                        if (alreadySignalled.syncContext == syncContext) {
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        usedSignals.push_back(SignalDescription {
                            nodes[write.node].inputDependency[write.dependencyIndex].state.accessStage,
                            syncContext
                        });
                    }
                }
            }

            for (int i = 0; i < queuesCount; i++) {
                if (perQueueSignals[i].stage != 0)
                    usedSignals.push_back(perQueueSignals[i]);
            }

            if (usedSignals.size() > 0)
                queueTimelines[queueIndex].push_back(QueueTimeStep::Wait(std::move(usedSignals)));

            queueTimelines[queueIndex].push_back(QueueTimeStep::Node(node));

            if (syncContexts[node].timelineValue.timelineValue > 0)
                queueTimelines[queueIndex].push_back(QueueTimeStep::Signal(SignalDescription {
                    syncContexts[node].signalStage,
                    node
                }));
        }
    }

    void simplifyTimelines() {
        PerQueueStorage<uint64_t> perQueueSignals;
        std::unordered_map<ResourceId, uint32_t> perResourceSignals;

        for (uint32_t queue = 0; queue < (uint32_t)QueueType::None; queue++) {
            perQueueSignals.clear_to_zero();
            perResourceSignals.clear();
            auto it = queueTimelines[queue].begin();
            while (it != queueTimelines[queue].end()) {
                if (it->type == QueueTimeStep::Type::Wait) {
                    
                    for (int i = it->signals.size() - 1; i >= 0; --i) {
                        
                        SignalDescription signal = it->signals[i];
                        SynchronizationContext& context = syncContexts[signal.syncContext];

                        uint64_t& compareValue = perQueueSignals[context.timelineValue.queueIndex];
                        if (context.externalSyncSource.has_value())
                            compareValue = perResourceSignals[context.externalSyncSource.value()];

                        if (compareValue > context.timelineValue.timelineValue) {
                            it->signals.erase(it->signals.begin() + i);
                        }
                        else {
                            compareValue = context.timelineValue.timelineValue;
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

void EnqueueBarriers(const NodeWrapper& node, Allocator& alloc, TransferCommandBuffer& commandBuffer) 
{
    auto _ = alloc.BeginContext();
    MemBuffer<ResourceId> resources = alloc.BumpAllocate<ResourceId>(
        node.inputDependency.size + node.outpnputDependency.size);
    MemBuffer<ResourceState> states = alloc.BumpAllocate<ResourceState>(
        node.inputDependency.size + node.outpnputDependency.size);
    for (int i = 0; i < node.inputDependency.size; i++) {
        
        if (node.inputDependency[i].stateByWriter)
            continue;

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
}

void EnqueuePostBarriers(
    const GraphInstance& instance,
    uint32_t nodeId, Allocator& alloc, TransferCommandBuffer& commandBuffer) 
{
    auto _ = alloc.BeginContext();
    auto& writes = instance.outEdges[nodeId];

    MemBuffer<ResourceId> resources = alloc.BumpAllocate<ResourceId>(
        writes.size());
    MemBuffer<ResourceState> states = alloc.BumpAllocate<ResourceState>(
        writes.size());

    for (int i = 0; i < writes.size(); i++) {
        VersionedResource write = writes[i];

        if (write.id.type() != ResourceType::Buffer && write.id.type() != ResourceType::Image)
            continue;

        auto it = instance.forceWriteStates.find(write);
        if (it != instance.forceWriteStates.end()) {
            resources.push_back(write.id);
            states.push_back(it->second);
        }
    }

    commandBuffer.Barrier(resources.size(), resources.data(), states.data());
}

struct TimelineExecutionContext 
{
    uint32_t queueIndex;
    std::list<QueueTimeStep>& timeline;
    std::list<QueueTimeStep>::iterator& step;
    uint64_t& maxTimelineValue;
    std::vector<Ref<Semaphore>>& timelineSemaphores;
    std::vector<uint64_t>& initialSemaphoreValues;
    std::unordered_map<QueueTimelineValue, Ref<Semaphore>>& binarySemaphores;
};

void FillSemaphoreInfo(RenderContext& context,
    GraphInstance& instance,
    TimelineExecutionContext& timelineContext,
    MemBuffer<VkSemaphoreSubmitInfo>& semaphores, bool allowExtrnalSync,
    MemBuffer<Ref<Semaphore>>* usedSemaphores = nullptr
) {
    uint32_t count = timelineContext.step->signals.size();
    assert(count <= semaphores.capacity());
    assert(usedSemaphores == nullptr || usedSemaphores->capacity() >= count);

    for (int i = 0; i < count; i++) {
        SignalDescription wait = timelineContext.step->signals[i];
        SynchronizationContext& syncContext = instance.syncContexts[wait.syncContext];
        
        Ref<Semaphore> semaphore;
        uint64_t timelineValue = 0;

        bool hasExternal = allowExtrnalSync && syncContext.externalSyncSource.has_value();

        if (hasExternal || syncContext.forceBinary) {
            if (hasExternal)
                semaphore = context.Get<Resources>().ExtractSyncContext(syncContext.externalSyncSource.value());
            else {
                auto it = timelineContext.binarySemaphores.find(syncContext.timelineValue);
                if (it != timelineContext.binarySemaphores.end())
                    semaphore = it->second;
                else{
                    semaphore = context.Get<Synchronization>().BorrowSemaphore(false);
                    timelineContext.binarySemaphores[syncContext.timelineValue] = semaphore;
                }
            }
        }
        else {
            semaphore = timelineContext.timelineSemaphores[syncContext.timelineValue.queueIndex];
            timelineValue = timelineContext.initialSemaphoreValues[syncContext.timelineValue.queueIndex] + 
                            syncContext.timelineValue.timelineValue;

            if (timelineValue > timelineContext.maxTimelineValue && 
                syncContext.timelineValue.queueIndex == timelineContext.queueIndex)
                timelineContext.maxTimelineValue = timelineValue;
        }

        if (!semaphore.isNull()) {
            
            if (usedSemaphores != nullptr)
                usedSemaphores->push_back(semaphore);

            semaphores.push_back(VkSemaphoreSubmitInfo {
                VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                nullptr,
                semaphore->vk,
                timelineValue,
                wait.stage
            });
        }
    }
}

void RunTimeline(
    RenderContext& context,
    GraphInstance& instance,
    TimelineExecutionContext& timelineContext
) 
{
    Allocator& alloc = context.Get<Allocator>();
    auto _ = alloc.BeginContext();
    
    std::optional<TransferCommandBuffer> cmd;

    QueueType queueType = static_cast<QueueType>(timelineContext.queueIndex);

    if (queueType != QueueType::Present) {
        cmd = context.Get<CommandPool>().BorrowCommandBuffer(queueType);
        cmd.value().Begin();
    }

    MemBuffer<VkSemaphoreSubmitInfo> waitContext = MemChunk<VkSemaphoreSubmitInfo>::Null();
    MemBuffer<Ref<Semaphore>> waitSemaphores = MemChunk<Ref<Semaphore>>::Null();
    MemBuffer<VkSemaphoreSubmitInfo> signalContext = MemChunk<VkSemaphoreSubmitInfo>::Null();

    bool finalSubmit = true;

    do {
        if (timelineContext.step->type == QueueTimeStep::Type::Wait) {
            assert(waitContext.data() == nullptr);

            waitSemaphores = alloc.BumpAllocate<Ref<Semaphore>>(timelineContext.step->signals.size());
            waitContext = alloc.BumpAllocate<VkSemaphoreSubmitInfo>(timelineContext.step->signals.size());

            FillSemaphoreInfo(context, instance, timelineContext, waitContext, true, &waitSemaphores);

        }
        else if (timelineContext.step->type == QueueTimeStep::Type::Node) {
            NodeWrapper& node = instance.nodes[timelineContext.step->node];

            if (cmd.has_value()) {
                EnqueueBarriers(node, alloc, cmd.value());
            }

            TransferCommandBuffer* cmd_p = nullptr;
            if (cmd.has_value())
                cmd_p = &cmd.value();

            node.node->Record(ExecutionContext {
                cmd_p,
                waitSemaphores
            });   

            if (cmd.has_value()) {
                EnqueuePostBarriers(instance, timelineContext.step->node, alloc, cmd.value());
            }
        }
        else {

            assert(signalContext.data() == nullptr);

            signalContext = alloc.BumpAllocate<VkSemaphoreSubmitInfo>(timelineContext.step->signals.size() + 1);

            FillSemaphoreInfo(context, instance, timelineContext, signalContext, false);
            bool finalSubmit = std::next(timelineContext.step) == timelineContext.timeline.end();
            break;
        }

    } while(timelineContext.step != timelineContext.timeline.end());

    if (cmd.has_value())
        cmd.value().End();
        if (finalSubmit) {
            signalContext.push_back(VkSemaphoreSubmitInfo {
                VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                nullptr,
                timelineContext.timelineSemaphores[timelineContext.queueIndex]->vk,
                ++timelineContext.maxTimelineValue,
                VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT
            });
        }

        context.Get<CommandPool>().Submit(cmd.value(), 
            waitContext.size(),
            waitContext.data(),
            signalContext.size(),
            signalContext.data()
        );
}

void RunGraph(
    RenderContext& context, 
    GraphInstance& instance, 
    std::vector<Ref<Semaphore>>& semaphores,
    std::vector<uint64_t>& initialValuess
) {

    uint32_t queueCount = static_cast<size_t>(QueueType::None);
    PerQueueStorage<uint64_t> maxTimelineValues;
    std::unordered_map<QueueTimelineValue, Ref<Semaphore>> binarySemaphores;

    for (int queueIndex =0; queueIndex < queueCount; queueIndex++) {

        if (instance.queueTimelines[queueIndex].size() == 0)
            continue;
        
        for (QueueTimeStep& timelineEntry : instance.queueTimelines[queueIndex]) {

            TimelineExecutionContext executionContext {
                queueIndex,
                instance.queueTimelines[queueIndex],
                instance.queueTimelines[queueIndex].begin(),
                maxTimelineValues[queueIndex],
                semaphores,
                initialValuess,
                binarySemaphores
            };

            while (executionContext.step != instance.queueTimelines[queueIndex].end())
            {
                RunTimeline(context, instance, executionContext);
            }
        }
    }

    for (uint32_t i = 0; i < static_cast<uint32_t>(QueueType::None); i++) {
        initialValuess[i] = maxTimelineValues[i];
    }
}

void RenderGraph::OnMessage(BeginFrameMsg* m) {
    semaphoresPerQueue.SetFrame(m->inFlightFrame);
    semaphoreValuesPerQueue.SetFrame(m->inFlightFrame);

    if (semaphoresPerQueue->size() == 0) {
        uint32_t queueCount = static_cast<uint32_t>(QueueType::None);
        semaphoresPerQueue->reserve(queueCount);
        semaphoreValuesPerQueue->reserve(queueCount);

        for (int i = 0; i < queueCount; i++) {
            semaphoresPerQueue->push_back(context.Get<Synchronization>().CreateSemaphore(true));
            semaphoreValuesPerQueue->push_back(0);
        }
    } 
    else {
        Allocator& alloc = context.Get<Allocator>();
        auto _ = alloc.BeginContext();

        MemBuffer<VkSemaphore> semaphores = alloc.BumpAllocate<VkSemaphore>(semaphoresPerQueue->size());
        MemBuffer<uint64_t> values = alloc.BumpAllocate<uint64_t>(semaphoreValuesPerQueue->size());

        for (int i = 0; i < semaphoresPerQueue->size(); i++) {
            uint64_t value = semaphoreValuesPerQueue.val()[i]; 
            if (value == 0)
                continue;

            semaphores.push_back(semaphoresPerQueue.val()[i]->vk);
            values.push_back(value);
        }

        VkSemaphoreWaitInfo waitInfo {VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO};
        waitInfo.semaphoreCount = semaphores.size();
        waitInfo.pSemaphores = semaphores.data();
        waitInfo.pValues = values.data();

        vkWaitSemaphores(context.device(), &waitInfo, UINT64_MAX);
    }
}


void RenderGraph::Run() {

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
    instance.defineSyncronizationContexts(context);
    instance.buildTimelines();
    instance.simplifyTimelines();

    RunGraph(
        context, 
        instance, 
        *semaphoresPerQueue,
        *semaphoreValuesPerQueue
    );

    nodes.clear();
}