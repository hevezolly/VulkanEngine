#define DEBUG_RENDER_GRAPH
#include <render_context.h>
#include <render_graph.h>
#include <resource_id.h>

struct DebugNode: RenderNode {

    DebugNode(RenderContext& c, QueueType queueType) : RenderNode(c), queue(queueType){}

    virtual QueueType getTargetQueue() {return queue;}
    virtual uint32_t getInputDependenciesCount() {return inputDependency.size();}
    virtual uint32_t getOutputDependenciesCount() {return outputDependency.size();}
    virtual void getInputDependencies(NodeDependency* buffer) {
        for (ResourceId resource: inputDependency) {
            *buffer = NodeDependency{
                resource,
                ResourceState()
            };
            buffer++;
        }
    }
    virtual void getOutputDependencies(NodeDependency* buffer) {
        for (ResourceId resource: outputDependency) {
            *buffer = NodeDependency{
                resource,
                ResourceState()
            };
            buffer++;
        }
    }

    virtual void Record(ExecutionContext commandBuffer) {};

    DebugNode& AddInput(std::initializer_list<ResourceId> ids) {
        for (ResourceId id : ids) {
            inputDependency.push_back(id);
        }
        return *this;
    }

    DebugNode& AddOutput(std::initializer_list<ResourceId> ids) {
        for (ResourceId id : ids) {
            outputDependency.push_back(id);
        }
        return *this;
    }

    std::vector<ResourceId> inputDependency;
    std::vector<ResourceId> outputDependency;
    QueueType queue;
};

void main() {
    RenderContext context;
    context.WithFeature<RenderGraph>();
    Initialize(context);
 
    ResourceId id0 = ResourceId::Compose(ResourceType::Other, 0);
    ResourceId id1 = ResourceId::Compose(ResourceType::Other, 1);
    ResourceId id2 = ResourceId::Compose(ResourceType::Other, 2);
    ResourceId id3 = ResourceId::Compose(ResourceType::Other, 3);
    ResourceId id4 = ResourceId::Compose(ResourceType::Other, 4);
    ResourceId id5 = ResourceId::Compose(ResourceType::Other, 5);
    ResourceId id6 = ResourceId::Compose(ResourceType::Other, 6);

    RenderGraph& graph = context.Get<RenderGraph>();

    graph.AddNode<DebugNode>(QueueType::Graphics)
        .AddInput({id0})
        .AddOutput({id1})
        .SetName("node1");

    graph.AddNode<DebugNode>(QueueType::Graphics)
        .AddInput({id1})
        .AddOutput({id2})
        .SetName("node2");

    graph.Run();
}