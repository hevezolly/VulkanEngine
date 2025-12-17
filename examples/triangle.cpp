#include <iostream>
#include <render_context.h>
#include <shader_source.h>
#include <present_feature.h>
#include <graphics_feature.h>
#include <command_pool.h>

void RecordCommandBuffer(CommandBuffer& cmd, Ref<GraphicsPipeline> pipeline, Ref<FrameBuffer> frameBuffer) {
    cmd.Begin();
    cmd.BeginRenderPass(pipeline, frameBuffer);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(frameBuffer->width);
    viewport.height = static_cast<float>(frameBuffer->height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd.buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {frameBuffer->width, frameBuffer->height};
    vkCmdSetScissor(cmd.buffer, 0, 1, &scissor);

    vkCmdDraw(cmd.buffer, 3, 1, 0, 0);

    cmd.EndRenderPass();
    cmd.End();
}

void DrawFrame(
    RenderContext& context,
    CommandBuffer& cmd, 
    Ref<Semaphore> imgAvailable, 
    std::vector<Ref<Semaphore>>& renderFinish, 
    Ref<Fence> inFlight,
    Ref<GraphicsPipeline> pipeline,
    std::vector<Ref<FrameBuffer>>& buffers
) {
    inFlight->Wait();
    inFlight->Reset();

    uint32_t imageIndex = context.Get<PresentFeature>().swapChain->AcquireNextImage(imgAvailable);

    cmd.Reset();
    RecordCommandBuffer(cmd, pipeline, buffers[imageIndex]);

    Ref<Semaphore> renderInCurrentImageFinish = renderFinish[imageIndex];

    context.Get<CommandPool>().Submit(cmd, imgAvailable, renderInCurrentImageFinish, inFlight);

    context.Get<PresentFeature>().Present(imageIndex, renderInCurrentImageFinish);
}

void Run() {

    WindowInitializer windowDescription{};
    SwapChainInitializer swapChainDescription{};
    windowDescription.width = 800;
    windowDescription.height = 600;
    windowDescription.hint = "VkEngine";
    RenderContext context;
    context.WithFeature<PresentFeature>(windowDescription, swapChainDescription)
           .WithFeature<GraphicsFeature>();
    context.Initialize();

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

    auto pipeline = context
        .Get<GraphicsFeature>().GraphicsPipeline()
        .AddShaderStage(ShaderStage::Vertex, vertexBin)
        .AddShaderStage(ShaderStage::Pixel, fragmentBin)
        .AddDynamicState(VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT)
        .AddDynamicState(VkDynamicState::VK_DYNAMIC_STATE_SCISSOR)
        .Build();
    
    Ref<Semaphore> imageAvailable = context.Get<Synchronization>().CreateSemaphore();
    Ref<Fence> inFlight = context.Get<Synchronization>().CreateFence(true);
    
    std::vector<Ref<Semaphore>> renderFinish;
    renderFinish.reserve(swapChain->images.size());
    std::vector<Ref<FrameBuffer>> frameBuffers;
    frameBuffers.reserve(swapChain->images.size());
    for (int i = 0; i < swapChain->images.size(); i++) {
        renderFinish.push_back(context.Get<Synchronization>().CreateSemaphore());
        frameBuffers.push_back(pipeline->CreateFrameBuffer(swapChain->images[i].view));
    }

    CommandBuffer cmd = context.Get<CommandPool>().CreateGraphicsBuffer();

    while (!glfwWindowShouldClose(context.Get<PresentFeature>().window->pWindow)) {
        glfwPollEvents();
        DrawFrame(
            context, 
            cmd, 
            imageAvailable, 
            renderFinish, 
            inFlight, 
            pipeline,
            frameBuffers
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