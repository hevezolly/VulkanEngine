#define THROW_ON_UNDESIRED_SWAPCHAIN
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
#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <thread>
#include <stable_frame_rate.h>
#include <registry.h>

#define BLOCK_NAME Vertex
#define BLOCK \
VEC3(position, 0) \
VEC3(color, 1) \
VEC2(uv, 2)
#include <gen_vertex_data.h>

/*
expands to:
struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 uv;

    //... other required binding information
};
*/

#define BLOCK_NAME ShaderInput
#define BLOCK \
UNIFORM_BUFFER(transforms, 0, Stage::Vertex) \
IMAGE_SAMPLER(img, 1, Stage::Fragment)
#include <gen_bindings.h>

/*
expands to:
struct ShaderInput {
    Buffer* transforms;
    ImageView* img;
    Sampler* img_sampler;

    //... other required binding information
};
*/

#define BLOCK_NAME Attachments
#define BLOCK \
COLOR(color, LoadOp::Clear) FINAL_LAYOUT(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) \
DEPTH(depth, LoadOp::Clear) 
#include <gen_attachments.h>

/*
expands to:
struct ShaderInput {
    ImageView* color;
    ImageView* depthl

    struct Formats {
        VkFormat color;
        VkFormat depth;
    };

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
    ResourceRefs<Buffer> uniformBuffers;
    Ref<GraphicsPipeline> pipeline;
    ResourceRef<Buffer> vertexBuffer;
    ResourceRef<Buffer> indexBuffer;
    ResourceRef<Image> image;
    ResourceRef<Image> depth;
    ResourceRef<Sampler> sampler;
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

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 uv;
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 uv_out;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(in_position, 1.0);
    fragColor = in_color;
    uv_out = uv;
})";

    ShaderSource fragmentSource;
    fragmentSource.name = "testFragment";
    fragmentSource.stage = Stage::Fragment;
    fragmentSource.source = 
R"(#version 450
layout(binding = 1) uniform sampler2D tex;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 uv;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(tex, uv);
}
)";

    ShaderCompiler compiler;
    ShaderBinary vertexBin = compiler.FromSource(vertexSource);
    ShaderBinary fragmentBin = compiler.FromSource(fragmentSource);

    VkFormat dsFormat = context.Get<Device>().SelectSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    r.depth = context.Get<Resources>().CreateImage({dsFormat, context.Get<PresentFeature>().swapChainExtent()}, 
        ImageUsage::DepthStencil);
    r.image = context.Get<Resources>().LoadImage(ImageUsage::Sampled, "test_img.png", VK_FORMAT_R8G8B8A8_SRGB);


    r.pipeline = context
        .Get<GraphicsFeature>().NewGraphicsPipeline()
        .SetVertex<Vertex>()
        .AddLayout<ShaderInput>()
        .SetAttachments<Attachments>(Attachments::Formats{
            context.Get<PresentFeature>().swapChain->format,
            r.depth->description.format
        })
        .AddShaderStage(Stage::Vertex, vertexBin)
        .AddShaderStage(Stage::Fragment, fragmentBin)
        .AddDynamicState(VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT)
        .AddDynamicState(VkDynamicState::VK_DYNAMIC_STATE_SCISSOR)
        .Build();

    std::vector<Vertex> vertices {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
    };
    r.vertexBuffer = context.Get<Resources>().CreateBuffer<Vertex>(BufferPreset::VERTEX, vertices);

    std::vector<uint16_t> indices = {
        0, 2, 1, 2, 0, 3,
        4, 6, 5, 6, 4, 7
    };
    r.indexBuffer = context.Get<Resources>().CreateBuffer<uint16_t>(BufferPreset::INDEX, indices);
    
    r.imgAvailableSemaphores = context.Get<Synchronization>().CreateSemaphores(framesInFlight);
    r.inFlightFences = context.Get<Synchronization>().CreateFences(framesInFlight, true);
    r.renderFinish = context.Get<Synchronization>().CreateSemaphores(
        context.Get<PresentFeature>().swapChain->images.size());
    r.commandBuffers.reserve(framesInFlight);

    r.sampler = context.Get<Resources>().CreateSampler(SamplerFilter::LINEAR, SamplerAddressMode::REPEAT);

    r.uniformBuffers.reserve(framesInFlight);
    for (int i = 0; i < framesInFlight; i++) {
        r.commandBuffers.push_back(context.Get<CommandPool>().CreateGraphicsBuffer());
        r.uniformBuffers.push_back(context.Get<Resources>()
            .CreateBuffer<UniformData>(BufferPreset::UNIFORM, 1));
    }
      
    r.commandBuffers[0].Begin();

    r.commandBuffers[0].ImageBarrier(
        r.image, 
        ResourceState{
            VK_ACCESS_2_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        });

    r.commandBuffers[0].End();

    context.Get<CommandPool>().Submit(r.commandBuffers[0]);
    vkQueueWaitIdle(context.Get<Device>().queues.get(QueueType::Graphics));

    return r;
}

void RecordCommandBuffer(
    GraphicsCommandBuffer& cmd, 
    ResourceRef<Buffer> vertexBuffer,
    ResourceRef<Buffer> indexBuffer,
    Ref<GraphicsPipeline> pipeline, 
    const FrameBuffer& frameBuffer,
    VkDescriptorSet descriptorSet
) {
    cmd.Begin();
    cmd.BeginRenderPass(pipeline, frameBuffer);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(frameBuffer.width);
    viewport.height = static_cast<float>(frameBuffer.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd.buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {frameBuffer.width, frameBuffer.height};
    vkCmdSetScissor(cmd.buffer, 0, 1, &scissor);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd.buffer, 0, 1, &vertexBuffer->vkBuffer, &offset);

    vkCmdBindIndexBuffer(cmd.buffer, indexBuffer->vkBuffer, 0, VkIndexType::VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(cmd.buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 0, 1, &descriptorSet, 0, nullptr);

    vkCmdDrawIndexed(cmd.buffer, indexBuffer->count<uint16_t>(), 1, 0, 0, 0);

    cmd.EndRenderPass();
    cmd.End();
}

void UpdateShaderData(RenderContext& context, Buffer& uniformBuffer) {
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

    bool menuActive = true;

    Ref<Semaphore> imgAvailable = r.imgAvailableSemaphores[frameId];

    uint32_t imageIndex = context.Get<PresentFeature>().AcquireNextImage(imgAvailable);

    GraphicsCommandBuffer& cmd = r.commandBuffers[frameId];

    UpdateShaderData(context, *r.uniformBuffers[frameId]);

    vkDeviceWaitIdle(context.device());

    ShaderInput data{};
    data.transforms = r.uniformBuffers.back();
    data.img = r.image;
    data.img_sampler = r.sampler;

    DescriptorSet& set = context.Get<Descriptors>().BorrowDescriptorSet(data);

    Attachments attachments;
    attachments.color = context.Get<PresentFeature>().swapChain->images[imageIndex];
    attachments.depth = r.depth;
    
    const FrameBuffer& buffer = context.Get<GraphicsFeature>()
        .CreateFrameBuffer(attachments, r.pipeline->renderPass);

    cmd.Reset();
    RecordCommandBuffer(cmd, r.vertexBuffer, r.indexBuffer, r.pipeline, 
        buffer, set.vkSet
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
    swapChainDescription.desiredPresentMode = {VK_PRESENT_MODE_IMMEDIATE_KHR};
    windowDescription.width = 800;
    windowDescription.height = 600;
    windowDescription.hint = "VkEngine";
    RenderContext context;
    context.WithFeature<PresentFeature>(windowDescription, swapChainDescription)
           .WithFeature<GraphicsFeature>()
           .WithFeature<StableFPS>(60)
           .WithFeature<Registry>("examples/resources")
           .Initialize();
    volkLoadInstance(context.vkInstance);

    context.Get<Descriptors>().Preallocate<ShaderInput>(3);

    const uint32_t framesInFlight = 1;

    _Resources resources = PrepareResources(context, framesInFlight);
    glfwSwapInterval(1);
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