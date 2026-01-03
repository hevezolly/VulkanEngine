#pragma once

#include <render_context.h>
#include <shader_source.h>
#include <present_feature.h>
#include <graphics_feature.h>
#include <command_pool.h>

struct Resources {
    std::vector<GraphicsCommandBuffer> commandBuffers;
    Refs<Semaphore> imgAvailableSemaphores;
    Refs<Semaphore> renderFinish;
    Refs<Fence> inFlightFences;
    Ref<GraphicsPipeline> pipeline;
};

Resources PrepareResources(
    RenderContext& context, 
    const uint32_t framesInFlight, 
    std::string* vertexShader = nullptr,
    std::string* fragmentShader = nullptr) {

    Resources r{};

    ShaderSource vertexSource;
    vertexSource.name = "testVertex";
    vertexSource.stage = Stage::Vertex;
    vertexSource.source = (vertexShader) ? *vertexShader :
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
    fragmentSource.stage = Stage::Fragment;
    fragmentSource.source = (fragmentShader) ? *fragmentShader :
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

    r.pipeline = context
        .Get<GraphicsFeature>().NewGraphicsPipeline()
        .AddShaderStage(Stage::Vertex, vertexBin)
        .AddShaderStage(Stage::Fragment, fragmentBin)
        .AddDynamicState(VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT)
        .AddDynamicState(VkDynamicState::VK_DYNAMIC_STATE_SCISSOR)
        .Build();

    LOG("pipeline built")
    
    r.imgAvailableSemaphores = context.Get<Synchronization>().CreateSemaphores(framesInFlight);
    LOG("image available semaphores")
    r.inFlightFences = context.Get<Synchronization>().CreateFences(framesInFlight, true);
    LOG("in flight fences")
    r.renderFinish = context.Get<Synchronization>().CreateSemaphores(
        context.Get<PresentFeature>().swapChain->images.size());
    LOG("render finish fences")

    r.commandBuffers.reserve(framesInFlight);

    for (int i = 0; i < framesInFlight; i++) {
        r.commandBuffers.push_back(context.Get<CommandPool>().CreateGraphicsBuffer());
    }

    return r;
}

void RecordCommandBuffer(
    GraphicsCommandBuffer& cmd, 
    Ref<GraphicsPipeline> pipeline, 
    Ref<FrameBuffer> frameBuffer
) {
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
    Resources& r,
    uint32_t frameId
) {
    LOG("begin frame " << frameId)
    Ref<Fence> inFlight = r.inFlightFences[frameId];
    inFlight->Wait();
    inFlight->Reset();

    Ref<Semaphore> imgAvailable = r.imgAvailableSemaphores[frameId];

    uint32_t imageIndex = context.Get<PresentFeature>().AcquireNextImage(imgAvailable);
    LOG("acquire next image "<< imageIndex)

    GraphicsCommandBuffer& cmd = r.commandBuffers[frameId];

    cmd.Reset();
    RecordCommandBuffer(cmd, r.pipeline, 
        context.Get<PresentFeature>().GetFrameBuffer(imageIndex, r.pipeline->renderPass));

    Ref<Semaphore> renderInCurrentImageFinish = r.renderFinish[imageIndex];

    context.Get<CommandPool>().Submit(cmd, 
        {{imgAvailable, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}}, 
        {renderInCurrentImageFinish}, 
        inFlight
    );

    context.Send(PresentMsg{imageIndex, renderInCurrentImageFinish});
}