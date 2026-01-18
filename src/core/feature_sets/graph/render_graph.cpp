#include <render_graph.h>
#include <render_context.h>

void RenderGraph::AddNode(RenderNode& node) {

    NodeWrapper wrapper;
    wrapper.inputDependency = node.getInputDependencies();
    wrapper.outpnputDependency = node.getOutputDependencies();
    wrapper.node = &node;
    wrapper.inputVersions = context.Get<Allocator>().BumpAllocate<uint32_t>(wrapper.inputDependency.size);
    wrapper.outputVersions = context.Get<Allocator>().BumpAllocate<uint32_t>(wrapper.outputVersions.size);

    for (int i = 0; i < wrapper.inputDependency.size; i++) {
        wrapper.inputVersions[i] = _versions[wrapper.inputDependency[i].resource];
    }

    for (int i = 0; i < wrapper.outpnputDependency.size; i++) {
        wrapper.outputVersions[i] = ++_versions[wrapper.outpnputDependency[i].resource];
    }
}