#include <iostream>
#include <render_context.h>
#include <shader_source.h>
#include <present_feature.h>
#include <graphics_feature.h>
#include <command_pool.h>

void RecordCommandBuffer(CommandBuffer& cmd, Ref<GraphicsPipeline> pipeline, Ref<FrameBuffer> frameBuffer) {
    cmd.Begin();
    LOG("cmd begin")
    cmd.BeginRenderPass(pipeline, frameBuffer);
    LOG("render pass begin")

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(frameBuffer->width);
    viewport.height = static_cast<float>(frameBuffer->height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd.buffer, 0, 1, &viewport);
    LOG("render pass set viewport")

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {frameBuffer->width, frameBuffer->height};
    vkCmdSetScissor(cmd.buffer, 0, 1, &scissor);
    LOG("render pass set scissor")


    vkCmdDraw(cmd.buffer, 3, 1, 0, 0);
    LOG("render pass draw")

    cmd.EndRenderPass();
    LOG("render pass end")
    cmd.End();
    LOG("cmd end")
}

void DrawFrame(
    RenderContext& context,
    std::vector<CommandBuffer>& commandBuffers, 
    Refs<Semaphore>& imgAvailableSemaphores, 
    Refs<Semaphore>& renderFinish, 
    Refs<Fence>& inFlightFences,
    Ref<GraphicsPipeline> pipeline,
    uint32_t frameId
) {
    LOG("begin frame " << frameId)
    Ref<Fence> inFlight = inFlightFences[frameId];
    inFlight->Wait();
    LOG("wait")
    inFlight->Reset();
    LOG("reset")

    Ref<Semaphore> imgAvailable = imgAvailableSemaphores[frameId];

    uint32_t imageIndex = context.Get<PresentFeature>().AcquireNextImage(imgAvailable);
    LOG("acquire next image")

    CommandBuffer& cmd = commandBuffers[frameId];

    cmd.Reset();
    LOG("cmd reset")
    RecordCommandBuffer(cmd, pipeline, 
        context.Get<PresentFeature>().GetFrameBuffer(imageIndex, pipeline->renderPass));
    LOG("cmd record")

    Ref<Semaphore> renderInCurrentImageFinish = renderFinish[imageIndex];

    context.Get<CommandPool>().Submit(cmd, imgAvailable, renderInCurrentImageFinish, inFlight);
    LOG("commands submit")

    context.Get<PresentFeature>().Present(imageIndex, renderInCurrentImageFinish);
    LOG("end frame " << frameId)
}

void Run() {

    volkInitialize();

    WindowInitializer windowDescription{};
    SwapChainInitializer swapChainDescription{};
    windowDescription.width = 800;
    windowDescription.height = 600;
    windowDescription.hint = "VkEngine";
    RenderContext context;
    context.WithFeature<PresentFeature>(windowDescription, swapChainDescription)
           .WithFeature<GraphicsFeature>()
           .Initialize();
    volkLoadInstance(context.vkInstance);
    LOG("init")

    SwapChain* swapChain = context.Get<PresentFeature>().swapChain;

    ShaderSource vertexSource;
    vertexSource.name = "testVertex";
    vertexSource.stage = ShaderStage::Vertex;
    vertexSource.source = 
R"(#version 450
vec2 positions[3] = vec2[](
vec2(0.0, -0.5),
vec2(0.5, 0.5),
vec2(-0.5, 0.5)
);

void main() {
gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
})";

    ShaderSource fragmentSource;
    fragmentSource.name = "testFragment";
    fragmentSource.stage = ShaderStage::Pixel;
    fragmentSource.source = 
R"(#version 450

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
}
)";

    ShaderCompiler compiler;
    ShaderBinary vertexBin = compiler.FromSource(vertexSource);
    ShaderBinary fragmentBin = compiler.FromSource(fragmentSource);

    LOG("shaders compiled")

    auto pipeline = context
        .Get<GraphicsFeature>().GraphicsPipeline()
        .AddShaderStage(ShaderStage::Vertex, vertexBin)
        .AddShaderStage(ShaderStage::Pixel, fragmentBin)
        .AddDynamicState(VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT)
        .AddDynamicState(VkDynamicState::VK_DYNAMIC_STATE_SCISSOR)
        .Build();

    LOG("pipeline built")

    const uint32_t framesInFlight = 3;
    
    Refs<Semaphore> imageAvailable = context.Get<Synchronization>().CreateSemaphores(framesInFlight);
    LOG("image available semaphores")
    Refs<Fence> inFlight = context.Get<Synchronization>().CreateFences(framesInFlight, true);
    LOG("in flight fences")
    Refs<Semaphore> renderFinish = context.Get<Synchronization>().CreateSemaphores(swapChain->images.size());
    LOG("render finish fences")

    std::vector<CommandBuffer> cmds;
    cmds.reserve(framesInFlight);

    for (int i = 0; i < framesInFlight; i++) {
        cmds.push_back(context.Get<CommandPool>().CreateGraphicsBuffer());
    }

    LOG("command buffer built")

    uint32_t currentFrame = 0;
    while (!glfwWindowShouldClose(context.Get<PresentFeature>().window->pWindow)) {
        glfwPollEvents();
        LOG("poll events")
        DrawFrame(
            context, 
            cmds, 
            imageAvailable, 
            renderFinish, 
            inFlight, 
            pipeline,
            (currentFrame++) % framesInFlight
        );
    }
}

int main() {
    try {
        Run();
    } catch (const std::exception &e) {
        std::cout << e.what() << std::endl;
        throw;
    }

    std::cout << "Success!" << std::endl;

    return 0;
}