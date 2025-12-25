#include <iostream>
#include <render_context.h>
#include <shader_source.h>
#include <present_feature.h>
#include <graphics_feature.h>
#include <descriptor_pool.h>
#include <command_pool.h>
#include <resources.h>
#include <allocator_feature.h>
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>
#include <thread>

#define BLOCK_NAME Vertex
#define BLOCK \
VEC2(position, 0) \
VEC3(color, 1)
#include <gen_vertex_data.h>

/*
expands to:
struct Vertex {
    glm::vec2 position;
    glm::vec3 color;

    //... other required binding information
};
*/

#define BLOCK_NAME ShaderInput
#define BLOCK \
UNIFORM_BUFFER(transforms, 0, Stage::Vertex)
#include <gen_bindings.h>

/*
expands to:
struct ShaderInput {
    Buffer* transforms;

    //... other required binding information
};
*/

struct UniformData {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct _Resources {
    std::vector<GraphicsCommandBuffer> commandBuffers;
    Refs<Semaphore> imgAvailableSemaphores;
    Refs<Semaphore> renderFinish;
    Refs<Fence> inFlightFences;
    Refs<Buffer> uniformBuffers;
    Ref<GraphicsPipeline> pipeline;
    Ref<Buffer> vertexBuffer;
    Ref<Buffer> indexBuffer;
};

_Resources PrepareResources(
    RenderContext& context, 
    const uint32_t framesInFlight) {

    _Resources r{};

    ShaderSource vertexSource;
    vertexSource.name = "testVertex";
    vertexSource.stage = Stage::Vertex;
    vertexSource.source = 
R"(#version 450
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(in_position, 0.0, 1.0);
    fragColor = in_color;
})";

    ShaderSource fragmentSource;
    fragmentSource.name = "testFragment";
    fragmentSource.stage = Stage::Fragment;
    fragmentSource.source = 
R"(#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1);
}
)";

    ShaderCompiler compiler;
    ShaderBinary vertexBin = compiler.FromSource(vertexSource);
    ShaderBinary fragmentBin = compiler.FromSource(fragmentSource);

    r.pipeline = context
        .Get<GraphicsFeature>().GraphicsPipeline()
        .SetVertex<Vertex>()
        .AddLayout<ShaderInput>()
        .AddShaderStage(Stage::Vertex, vertexBin)
        .AddShaderStage(Stage::Fragment, fragmentBin)
        .AddDynamicState(VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT)
        .AddDynamicState(VkDynamicState::VK_DYNAMIC_STATE_SCISSOR)
        .Build();

    std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
    };
    r.vertexBuffer = context.Get<Resources>().CreateBuffer<Vertex>(BufferPreset::VERTEX, vertices);

    std::vector<uint16_t> indices = {
        0, 2, 1, 2, 0, 3
    };
    r.indexBuffer = context.Get<Resources>().CreateBuffer<uint16_t>(BufferPreset::INDEX, indices);
    
    r.imgAvailableSemaphores = context.Get<Synchronization>().CreateSemaphores(framesInFlight);
    r.inFlightFences = context.Get<Synchronization>().CreateFences(framesInFlight, true);
    r.renderFinish = context.Get<Synchronization>().CreateSemaphores(
        context.Get<PresentFeature>().swapChain->images.size());
    r.commandBuffers.reserve(framesInFlight);

    for (int i = 0; i < framesInFlight; i++) {
        r.commandBuffers.push_back(context.Get<CommandPool>().CreateGraphicsBuffer());
        r.uniformBuffers.push_back(context.Get<Resources>()
            .CreateBuffer<UniformData>(BufferPreset::UNIFORM, 1));
        
    }

    return r;
}

void RecordCommandBuffer(
    GraphicsCommandBuffer& cmd, 
    Ref<Buffer> vertexBuffer,
    Ref<Buffer> indexBuffer,
    Ref<GraphicsPipeline> pipeline, 
    Ref<FrameBuffer> frameBuffer,
    VkDescriptorSet descriptorSet
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

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd.buffer, 0, 1, &vertexBuffer->vkBuffer, &offset);

    vkCmdBindIndexBuffer(cmd.buffer, indexBuffer->vkBuffer, 0, VkIndexType::VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(cmd.buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 0, 1, &descriptorSet, 0, nullptr);

    vkCmdDrawIndexed(cmd.buffer, indexBuffer->count<uint16_t>(), 1, 0, 0, 0);

    cmd.EndRenderPass();
    cmd.End();
}

VkDescriptorSet UpdateShaderData(RenderContext& context, Buffer& uniformBuffer) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    SwapChain* swapChain = context.Get<PresentFeature>().swapChain;
    UniformData d{};
    d.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    d.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    d.proj = glm::perspective(glm::radians(45.0f), swapChain->extent.width / (float) swapChain->extent.height, 0.1f, 10.0f);
    
    d.proj[1][1] *= -1;

    memcpy(uniformBuffer.memory.PersistentMap().data(), &d, sizeof(d));

    ShaderInput input{};
    input.transforms = &uniformBuffer;

    return context.Get<Descriptors>().UpdateDescriptorSet(input);
} 

void DrawFrame(
    RenderContext& context,
    _Resources& r,
    uint32_t frameId
) {
    Ref<Fence> inFlight = r.inFlightFences[frameId];
    inFlight->Wait();
    inFlight->Reset();

    context.Send(BeginFrameMsg{frameId});

    Ref<Semaphore> imgAvailable = r.imgAvailableSemaphores[frameId];

    uint32_t imageIndex = context.Get<PresentFeature>().AcquireNextImage(imgAvailable);

    GraphicsCommandBuffer& cmd = r.commandBuffers[frameId];

    VkDescriptorSet set = UpdateShaderData(context, *r.uniformBuffers[frameId]);

    cmd.Reset();
    RecordCommandBuffer(cmd, r.vertexBuffer, r.indexBuffer, r.pipeline, 
        context.Get<PresentFeature>().GetFrameBuffer(imageIndex, r.pipeline->renderPass),
        set
    );

    Ref<Semaphore> renderInCurrentImageFinish = r.renderFinish[imageIndex];

    context.Get<CommandPool>().Submit(cmd, 
        {{imgAvailable, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}}, 
        {renderInCurrentImageFinish}, 
        inFlight
    );

    context.Send(PresentMsg{imageIndex, renderInCurrentImageFinish});
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

    context.Get<Descriptors>().Preallocate<ShaderInput>(3);

    const uint32_t framesInFlight = 3;



    _Resources resources = PrepareResources(context, framesInFlight);

    uint32_t currentFrame = 0;
    while (!glfwWindowShouldClose(context.Get<PresentFeature>().window->pWindow)) {
        glfwPollEvents();
        DrawFrame(
            context, 
            resources,
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