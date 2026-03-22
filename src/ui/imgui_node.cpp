#include <imgui_node.h>
#include <render_context.h>

ImguiNode::ImguiNode(RenderContext& ctx, ResourceRef<Image> output): 
RenderNode(ctx), outputImg(output) 
{
    ASSERT(context.Has<ImguiUI>());
}

void ImguiNode::Record(ExecutionContext c) {
    context.Get<ImguiUI>().Record(outputImg, 
        *static_cast<GraphicsCommandBuffer*>(c.commandBuffer));
}