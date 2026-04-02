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

std::string queueName(QueueType type) {
    switch (type)
    {
        case QueueType::Graphics:
            return "G";
        case QueueType::Compute:
            return "C";
        case QueueType::Transfer:
            return "T";
        case QueueType::Present:
            return "P";
        default:
            return "?";
    }
}

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
    bool populated;

    SignalDescription(): stage(0), syncContext(0), populated(false){}
    SignalDescription(VkPipelineStageFlags2 s, uint32_t c): stage(s), syncContext(c), populated(true){}
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
    uint64_t timelineValue;
    VkPipelineStageFlags2 signalStage;
    uint32_t queue;
    SemaphoreRequirements semaphoreRequirements;
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
                    ASSERT(initialNodes.size() <= capacity-1);
                    initialNodes.push_back(m);
                }
            }
        }

        for (int i = 0; i < capacity; i++) {
            ASSERT(outcomingEdges[i].size() == 0);
            ASSERT(incomingEdges[i].size() == 0);
        }

        for (int i = 0; i < sortedNodes.size(); i ++) {
            nodes[sortedNodes[i]].sortedIndex = i;
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
                        ++timelineValue,
                        VK_PIPELINE_STAGE_2_NONE,
                        queue,
                        SemaphoreRequirements::Timeline,
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
                syncContexts[node].timelineValue = ++semaphoreValues[queueIndex]; 
                syncContexts[node].queue = queueIndex;
                syncContexts[node].semaphoreRequirements = SemaphoreRequirements::Timeline;
                syncContexts[node].signalStage = stage;
            }

        }
    }

    void AssignWait(SignalDescription& signal, SignalDescription newSignal, SemaphoreRequirements semaphoreRequirements) {

        SynchronizationContext* oldContext = nullptr;
        SynchronizationContext* newContext = &syncContexts[newSignal.syncContext];
        
        if (signal.populated) {
            oldContext = &syncContexts[signal.syncContext];
        }

        if (oldContext == nullptr || oldContext->timelineValue < newContext->timelineValue) {
            if (oldContext != nullptr) {
                ASSERT(oldContext->waits > 0);
                oldContext->waits--;
            }

            newContext->waits++;
            signal.syncContext = newSignal.syncContext;
        }

        signal.stage |= newSignal.stage;
        signal.populated = true;
        if (static_cast<uint32_t>(semaphoreRequirements) >= 
            static_cast<uint32_t>(syncContexts[signal.syncContext].semaphoreRequirements))
            syncContexts[signal.syncContext].semaphoreRequirements = semaphoreRequirements;
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
                {
                    continue;
                }

                ResourceUsage write = it->second;

                auto extSync = externalSync.find(inResource.resource.id);
                if (extSync != externalSync.end()) {
                    ASSERT(extSync->second[queueIndex] != UINT32_MAX);

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

                AssignWait(perQueueSignals[newQueue], newSignal, nodes[node].node->getSemaphoreRequirements());
            }

            for (VersionedResource outResource: outEdges[node]) {
                auto extSync = externalSync.find(outResource.id);
                if (extSync != externalSync.end()) {
                    ResourceUsage write = writes[outResource];
                    ASSERT(extSync->second[queueIndex] != UINT32_MAX);
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
                            nodes[write.node].outpnputDependency[write.dependencyIndex].state.accessStage,
                            syncContext
                        });
                    }
                }
            }

            for (int i = 0; i < queuesCount; i++) {
                if (perQueueSignals[i].populated)
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
        PerQueueStorage<uint64_t> perQueueSignals;
        std::unordered_set<ResourceId> perResourceExternalSync;

        for (uint32_t queue = 0; queue < (uint32_t)QueueType::None; queue++) {
            perQueueSignals.clear_to_zero();
            perResourceExternalSync.clear();
            auto it = queueTimelines[queue].begin();
            while (it != queueTimelines[queue].end()) {
                if (it->type == QueueTimeStep::Type::Wait) {
                    
                    for (int i = it->signals.size() - 1; i >= 0; --i) {
                        
                        SignalDescription signal = it->signals[i];
                        SynchronizationContext& context = syncContexts[signal.syncContext];

                        uint64_t& compareValue = perQueueSignals[context.queue];
                        bool hasExternalSync = context.externalSyncSource.has_value();
                        bool inserted = true;
                        if (hasExternalSync)
                            inserted = perResourceExternalSync.insert(context.externalSyncSource.value()).second;

                        bool eraseCondition = (!hasExternalSync && compareValue > context.timelineValue)
                            || (hasExternalSync && !inserted);


                        if (eraseCondition) {
                            it->signals.erase(it->signals.begin() + i);
                        }
                        else if (!hasExternalSync) {
                            compareValue = context.timelineValue;
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
    std::list<QueueTimeStep>::iterator step;
    uint64_t& maxTimelineValue;
    std::vector<Ref<Semaphore>>& timelineSemaphores;
    std::vector<uint64_t>& initialSemaphoreValues;
    std::unordered_map<QueueTimelineValue, Ref<Semaphore>>& binarySemaphores;
    std::unordered_map<ResourceId, Ref<Semaphore>>& externalSyncContexts;

    Ref<Semaphore> GetExternalSync(RenderContext& context, ResourceId id) {
        auto it = externalSyncContexts.find(id);
        if (it != externalSyncContexts.end())
            return it->second;

        Ref<Semaphore> result = context.Get<Resources>().ExtractSyncContext(id);
        externalSyncContexts[id] = result;
        return result;
    }
};

void FillSemaphoreInfo(RenderContext& context,
    GraphInstance& instance,
    TimelineExecutionContext& timelineContext,
    MemBuffer<VkSemaphoreSubmitInfo>& semaphores, bool allowExtrnalSync,
    MemBuffer<Ref<Semaphore>>* usedSemaphores = nullptr
) {
    uint32_t count = timelineContext.step->signals.size();
    ASSERT(count <= semaphores.capacity());
    ASSERT(usedSemaphores == nullptr || usedSemaphores->capacity() >= count);

    for (int i = 0; i < count; i++) {
        SignalDescription wait = timelineContext.step->signals[i];
        SynchronizationContext& syncContext = instance.syncContexts[wait.syncContext];
        
        Ref<Semaphore> semaphore;
        uint64_t timelineValue = 0;

        bool hasExternal = syncContext.externalSyncSource.has_value();
        if (hasExternal && !allowExtrnalSync)
            continue;

        if (hasExternal || syncContext.semaphoreRequirements != SemaphoreRequirements::Timeline) {
            if (hasExternal)
                semaphore = timelineContext.GetExternalSync(context, syncContext.externalSyncSource.value());
            else {
                QueueTimelineValue key{syncContext.timelineValue, syncContext.queue};
                auto it = timelineContext.binarySemaphores.find(key);
                if (it != timelineContext.binarySemaphores.end())
                semaphore = it->second;
                else{
                    bool semaphorePerSwapChain = syncContext.semaphoreRequirements == SemaphoreRequirements::BinaryPerSwapchainImage;
                    semaphore = context.Get<Synchronization>().BorrowBinarySemaphore(semaphorePerSwapChain);
                    timelineContext.binarySemaphores[key] = semaphore;
                }
            }
        }
        else {
            semaphore = timelineContext.timelineSemaphores[syncContext.queue];
            timelineValue = timelineContext.initialSemaphoreValues[syncContext.queue] + 
                            syncContext.timelineValue;

            if (syncContext.timelineValue > timelineContext.maxTimelineValue && 
                syncContext.queue == timelineContext.queueIndex)
                timelineContext.maxTimelineValue = syncContext.timelineValue;
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
    
    std::unique_ptr<TransferCommandBuffer> cmd;

    QueueType queueType = static_cast<QueueType>(timelineContext.queueIndex);

    if (queueType != QueueType::Present) {
        cmd = context.Get<CommandPool>().BorrowCommandBuffer(queueType);
        cmd->Begin();
    }

    MemBuffer<VkSemaphoreSubmitInfo> waitContext = MemChunk<VkSemaphoreSubmitInfo>::Null();
    MemBuffer<Ref<Semaphore>> waitSemaphores = MemChunk<Ref<Semaphore>>::Null();
    MemBuffer<VkSemaphoreSubmitInfo> signalContext = MemChunk<VkSemaphoreSubmitInfo>::Null();

    bool finalSubmit = true;

    do {
        if (timelineContext.step->type == QueueTimeStep::Type::Wait) {
            ASSERT(waitContext.data() == nullptr);

            waitSemaphores = alloc.BumpAllocate<Ref<Semaphore>>(timelineContext.step->signals.size());
            waitContext = alloc.BumpAllocate<VkSemaphoreSubmitInfo>(timelineContext.step->signals.size());

            FillSemaphoreInfo(context, instance, timelineContext, waitContext, 
                queueType != QueueType::Present, &waitSemaphores);
            
        }
        else if (timelineContext.step->type == QueueTimeStep::Type::Node) {
            NodeWrapper& node = instance.nodes[timelineContext.step->node];
            if (cmd) {
                EnqueueBarriers(node, alloc, *cmd);
            }

            node.node->Record(ExecutionContext {
                cmd.get(),
                waitSemaphores
            });

            if (cmd) {
                EnqueuePostBarriers(instance, timelineContext.step->node, alloc, *cmd);
            }
        }
        else {

            ASSERT(signalContext.data() == nullptr);

            bool finalSubmit = std::next(timelineContext.step) == timelineContext.timeline.end();

            uint32_t allocationSize = timelineContext.step->signals.size();
            if (finalSubmit) {
                allocationSize++;
            }

            signalContext = alloc.BumpAllocate<VkSemaphoreSubmitInfo>(allocationSize);

            FillSemaphoreInfo(context, instance, timelineContext, signalContext, false);
            ++timelineContext.step;
            break;
        }

        ++timelineContext.step;

    } while(timelineContext.step != timelineContext.timeline.end());

    if (cmd) {
        cmd->End();
        
        if (finalSubmit) {
            
            if (signalContext.size() == 0)
                signalContext = alloc.BumpAllocate<VkSemaphoreSubmitInfo>();

            signalContext.push_back(VkSemaphoreSubmitInfo {
                VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                nullptr,
                timelineContext.timelineSemaphores[timelineContext.queueIndex]->vk,
                timelineContext.initialSemaphoreValues[timelineContext.queueIndex] + 
                    ++timelineContext.maxTimelineValue,
                VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT
            });
        }

        context.Get<CommandPool>().Submit(*cmd, 
            waitContext.size(),
            waitContext.data(),
            signalContext.size(),
            signalContext.data()
        );
    }
}

void DebugSignals(RenderContext& context,
    GraphInstance& instance,
    TimelineExecutionContext& timelineContext,
    std::stringstream& ss
) {
    for (SignalDescription s : timelineContext.step->signals) {
        SynchronizationContext& c = instance.syncContexts[s.syncContext];
        QueueType waitQueue = static_cast<QueueType>(c.queue);

        if (c.externalSyncSource.has_value()) {
            ss << "E(" << context.Get<Resources>().GetName(c.externalSyncSource.value()) << "),";
        }
        else {
            ss << queueName(waitQueue);
            if (c.semaphoreRequirements == SemaphoreRequirements::BinaryPerFrame) {
                ss << "*";
            } else if (c.semaphoreRequirements == SemaphoreRequirements::BinaryPerSwapchainImage) {
                ss << "!";
            }
            ss << "(" << c.timelineValue << "),";
        }

    }
}

void DebugTimeline(
    RenderContext& context,
    GraphInstance& instance,
    TimelineExecutionContext& timelineContext
) {
    std::stringstream ss;

    QueueType type = static_cast<QueueType>(timelineContext.queueIndex);
    std::string sep = "--";
    ss << queueName(type) << ":" << sep;

    do {
        if (timelineContext.step->type == QueueTimeStep::Type::Wait) {
            ss << "Wait(";
            DebugSignals(context, instance, timelineContext, ss);
            ss << ")" << sep;
        }
        else if (timelineContext.step->type == QueueTimeStep::Type::Node) {
            ss << "Node(" << instance.nodes[timelineContext.step->node].node->getName() << ")";
            ss << "(" << queueName(instance.nodes[timelineContext.step->node].queue) << ")";
            ss << "[" << instance.nodes[timelineContext.step->node].sortedIndex << "]";
            ss << sep;
        }
        else {
            ss << "Signal(";
            DebugSignals(context, instance, timelineContext, ss);
            ss << ")" << sep;
        }

        ++timelineContext.step;

    } while(timelineContext.step != timelineContext.timeline.end());
    std::cout << ss.str() << std::endl;
}

void RunGraph(
    RenderContext& context, 
    GraphInstance& instance, 
    std::vector<Ref<Semaphore>>& semaphores,
    std::vector<uint64_t>& initialValuess,
) {
    uint32_t queueCount = static_cast<size_t>(QueueType::None);
    PerQueueStorage<uint64_t> maxTimelineValues;
    std::unordered_map<QueueTimelineValue, Ref<Semaphore>> binarySemaphores;
    std::unordered_map<ResourceId, Ref<Semaphore>> externalSync;

    for (int queueIndex =0; queueIndex < queueCount; queueIndex++) {

        maxTimelineValues[queueIndex] = 0;
        if (instance.queueTimelines[queueIndex].size() == 0)
            continue;

        TimelineExecutionContext executionContext {
            queueIndex,
            instance.queueTimelines[queueIndex],
            instance.queueTimelines[queueIndex].begin(),
            maxTimelineValues[queueIndex],
            semaphores,
            initialValuess,
            binarySemaphores,
            externalSync
        };

        while (executionContext.step != instance.queueTimelines[queueIndex].end())
        {
#ifdef DEBUG_RENDER_GRAPH
            DebugTimeline(context, instance, executionContext);
#else
            RunTimeline(context, instance, executionContext);
#endif
        }
    }
    

    for (uint32_t i = 0; i < static_cast<uint32_t>(QueueType::None); i++) {
        initialValuess[i] += maxTimelineValues[i];
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
            context.NameVkObject(
                VkObjectType::VK_OBJECT_TYPE_SEMAPHORE, 
                (uint64_t)(semaphoresPerQueue->back()->vk), 
                "semaphore queue " + std::to_string(i) + " frame " + std::to_string(m->inFlightFrame));
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

    // index = 0;
    // for (NodeWrapper& node: nodes) {
    //     LOG("Node_" << index << " " << node.node->getName())
    //     LOG("Input dependencies:")
    //     for (int i = 0; i < node.inputDependency.size; i++) {
    //         auto& d = node.inputDependency[i];
    //         LOG("   " << context.Get<Resources>().GetName(d.resource) << " " << d.resource.id)
    //         LOG("   state: " << d.state.accessStage)
    //         LOG("   version: " << node.inputVersions[i])
    //         LOG("")
    //     }
    //     LOG("")
    //     LOG("Output dependencies:")
    //     for (int i = 0; i < node.outpnputDependency.size; i++) {
    //         auto& d = node.outpnputDependency[i];
    //         LOG("   " << context.Get<Resources>().GetName(d.resource) << " " << d.resource.id)
    //         LOG("   stage: " << d.state.accessStage)
    //         LOG("   version: " << node.outputVersions[i])
    //         LOG("")
    //     }
    //     LOG("")
    //     LOG("")
    //     index++;
    // }
    
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

    for (auto& node: nodes) {
        node.destructor(node.node);
    }

    nodes.clear();
}