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

int main() {
    RenderContext context;
    context.WithFeature<RenderGraph>();
    Initialize(context);
 
    ResourceId id0 = ResourceId::Compose(ResourceType::Other, 1);
    ResourceId id1 = ResourceId::Compose(ResourceType::Other, 2);
    ResourceId id2 = ResourceId::Compose(ResourceType::Other, 3);
    ResourceId id3 = ResourceId::Compose(ResourceType::Other, 4);
    ResourceId id4 = ResourceId::Compose(ResourceType::Other, 5);
    ResourceId id5 = ResourceId::Compose(ResourceType::Other, 6);
    ResourceId id6 = ResourceId::Compose(ResourceType::Other, 7);

    RenderGraph& graph = context.Get<RenderGraph>();

    std::cout << "simple graph" << std::endl; 
    graph.AddNode<DebugNode>(QueueType::Graphics)
        .AddInput({id0})
        .AddOutput({id1})
        .SetName("node1");

    graph.AddNode<DebugNode>(QueueType::Graphics)
        .AddInput({id1})
        .AddOutput({id2})
        .SetName("node2");

    graph.Debug();
    std::cout << std::endl;

// =====================================================
    
    std::cout << "write dependency" << std::endl; 
    
    graph.AddNode<DebugNode>(QueueType::Compute)
        .AddOutput({id1})
        .SetName("waitJob");

    graph.AddNode<DebugNode>(QueueType::Graphics)
        .AddInput({id1})
        .AddOutput({id2})
        .SetName("producer1");
    
    graph.AddNode<DebugNode>(QueueType::Graphics)
        .AddInput({id2})
        .AddOutput({id2})
        .SetName("producer2");
 
    graph.AddNode<DebugNode>(QueueType::Graphics)
        .AddOutput({id2, id3})
        .SetName("producer3");

    graph.Debug();
    std::cout << std::endl;

// =====================================================
    
    std::cout << "parallel execution" << std::endl; 
    
    graph.AddNode<DebugNode>(QueueType::Compute)
        .AddOutput({id1})
        .SetName("producer1");

    graph.AddNode<DebugNode>(QueueType::Graphics)
        .AddOutput({id2})
        .SetName("producer2");
    
    graph.AddNode<DebugNode>(QueueType::Graphics)
        .AddInput({id1, id2})
        .AddOutput({id3})
        .SetName("consumer");
 

    graph.Debug();
    std::cout << std::endl;
}