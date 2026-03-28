#define THROW_ON_UNDESIRED_SWAPCHAIN
#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan_engine.h>
#include <vulkan_engine_ui.h>

static const uint32_t PARTICLES_COUNT = 1024;
static const uint32_t THREAD_GROUP_SIZE = 32;

#define BLOCK_NAME Particle
#define BLOCK \
VEC2(position, 0) \
VEC2(velocity, 1) \
VEC4(color, 2)
#include <gen_vertex_data.h>

#define BLOCK_NAME Vertex
#define BLOCK \
VEC2(offset, 3)
#include <gen_vertex_data.h>

#define BLOCK_NAME Attachments
#define BLOCK \
COLOR(color, LoadOp::Clear)
#include <gen_attachments.h>

struct ParticleUniforms {
    float deltaTime;
    int particlesCount;
};

#define BLOCK_NAME ParticlesBindings
#define BLOCK \
DYNAMIC_UNIFORM(ubo, 0, Stage::Compute) \
SSBO(particles, 1, Stage::Compute, Access::ReadWrite)
#include <gen_bindings.h>


struct _Resources {
    Ref<ComputePipeline> computePipeline;
    Ref<GraphicsPipeline> graphicsPipeline;
    ResourceRef<Buffer> particlesBuffer;
    ResourceRef<Buffer> vertexBuffer;
    ResourceRef<Buffer> indexBuffer;
    std::chrono::steady_clock::time_point lastTime;
};

std::vector<Particle> generateParticlesBuffer() {
    std::vector<Particle> result;
    result.reserve(PARTICLES_COUNT);

    const float MinVelocity = 0.001;
    const float MaxVelocity = 0.04;
    
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> positionDist(-1.0f, 1.0f);
    std::uniform_real_distribution<float> colorDist(0, 1.0f);
    std::uniform_real_distribution<float> angleDist(0, 2.0f * 3.14159265);
    std::uniform_real_distribution<float> magnitudeDist(MinVelocity, MaxVelocity);

    
    for (int i = 0; i < PARTICLES_COUNT; i++) {
        
        Particle pa{};
        
        float angle = angleDist(rng);
        float magnitude = std::sqrt(magnitudeDist(rng));

        pa.velocity = magnitude * glm::vec2(std::cos(angle), std::sin(angle));
        pa.position.x = positionDist(rng);
        pa.position.y = positionDist(rng);

        pa.color.w = 1;
        pa.color.x = colorDist(rng);
        pa.color.y = colorDist(rng);
        pa.color.z = colorDist(rng);
    
        result.push_back(pa);
    }

    return result;
}

_Resources PrepareResources(
    RenderContext& context, 
    const uint32_t framesInFlight) {

    _Resources r{};
    
    ShaderBinary vertexBin = context.Get<ShaderLoader>().Get("shaders/particles.vert", Stage::Vertex);
    ShaderBinary fragmentBin = context.Get<ShaderLoader>().Get("shaders/particles.frag", Stage::Fragment);
    ShaderBinary computeBin = context.Get<ShaderLoader>().Get("shaders/particles.comp", Stage::Compute);
    
    r.graphicsPipeline = context
        .Get<GraphicsFeature>().NewGraphicsPipeline()
        .AddVertex<Vertex>(VertexInputRate::Vertex)
        .AddVertex<Particle>(VertexInputRate::Instance)
        .SetAttachments<Attachments>({
            context.Get<PresentFeature>().swapChain->format
        })
        .AddShaderStage(vertexBin)
        .AddShaderStage(fragmentBin)
        .AddDynamicState(VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT)
        .AddDynamicState(VkDynamicState::VK_DYNAMIC_STATE_SCISSOR)
        .Build();
    context.NameVkObject(VkObjectType::VK_OBJECT_TYPE_PIPELINE, (uint64_t)r.graphicsPipeline->pipeline, "graphics pipeline");

    r.computePipeline = context
        .Get<Compute>().NewComputePipeline()
        .AddShaderStage(computeBin)
        .AddLayout<ParticlesBindings>()
        .Build();
    context.NameVkObject(VkObjectType::VK_OBJECT_TYPE_PIPELINE, (uint64_t)r.computePipeline->pipeline, "compute pipeline");
    
    
    std::vector<Particle> particles = generateParticlesBuffer();


    r.particlesBuffer = context.Get<Resources>().CreateBuffer(
        BufferPreset::VERTEX | BufferPreset::STORAGE,
        particles
    );
    context.Get<Resources>().GiveName(r.particlesBuffer, "particlesBuffer");

    std::vector<Vertex> vertices = {
        {{-0.02, -0.02}},
        {{0.02, -0.02}},
        {{0.02, 0.02}},
        {{-0.02, 0.02}}
    };

    r.vertexBuffer = context.Get<Resources>().CreateBuffer(
        BufferPreset::VERTEX,
        vertices
    );
    context.Get<Resources>().GiveName(r.vertexBuffer, "vertexBuffer");

    std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0
    };

    r.indexBuffer = context.Get<Resources>().CreateBuffer(
        BufferPreset::INDEX,
        indices
    );
    context.Get<Resources>().GiveName(r.indexBuffer, "indexBuffer");

    r.lastTime = std::chrono::high_resolution_clock::now();

    return r;
}

void DrawFrame(
    RenderContext& context,
    _Resources& r
) {

    auto currentTume = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(
        currentTume - r.lastTime).count();
    r.lastTime = currentTume;
    
    context.BeginFrame();

    ImGui::ShowDemoWindow();

    ResourceRef<Image> outputImage = context.Get<PresentFeature>().AcquireNextImage();

    auto& updateParticlesNode = context.Get<RenderGraph>()
        .AddNode<ComputeNode<ParticlesBindings>>(r.computePipeline);
    updateParticlesNode.SetName("update particles node");

    updateParticlesNode.SetBindings(
        ParticlesBindings{
            context.Get<DynamicUniforms>().Allocate(ParticleUniforms{
                deltaTime,
                PARTICLES_COUNT
            }),
            r.particlesBuffer
        }
    );
    updateParticlesNode.SetGroups(
        (PARTICLES_COUNT + (THREAD_GROUP_SIZE - 1)) / THREAD_GROUP_SIZE
    );


    auto& drawParticlesNode = context.Get<RenderGraph>()
        .AddNode<GraphicsNode<Attachments>>(r.graphicsPipeline);
    drawParticlesNode.SetName("draw particles");
    drawParticlesNode.AddVertexBuffer(r.vertexBuffer);
    drawParticlesNode.AddVertexBuffer(r.particlesBuffer);
    drawParticlesNode.SetIndexBuffer(r.indexBuffer, VkIndexType::VK_INDEX_TYPE_UINT16);
    drawParticlesNode.SetAttachments(Attachments {
        outputImage
    });
    drawParticlesNode.AddDrawParameters(DrawParameters(4, 0, PARTICLES_COUNT, 0));

    
    context.Get<RenderGraph>().AddNode<ImguiNode>(outputImage).SetName("ui");
    context.Get<RenderGraph>().AddNode<PresentNode>(outputImage).SetName("present");

    context.Get<RenderGraph>().Run();
}

void Run() {

    WindowInitializer windowDescription{};
    SwapChainInitializer swapChainDescription{};
    swapChainDescription.desiredPresentMode = {VK_PRESENT_MODE_IMMEDIATE_KHR};
    windowDescription.width = 800;
    windowDescription.height = 600;
    windowDescription.hint = "VkEngine";
    
    const uint32_t framesInFlight = 3;
    
    RenderContext context;
    context.WithFeature<PresentFeature>(windowDescription, swapChainDescription)
           .WithFeature<FrameDispatcher>(framesInFlight)
           .WithFeature<GraphicsFeature>()
           .WithFeature<Compute>()
           .WithFeature<Registry>("examples/resources")
           .WithFeature<RenderGraph>()
           .WithFeature<DynamicUniforms>()
           .WithFeature<ImguiUI>();
    Initialize(context);


    _Resources resources = PrepareResources(context, framesInFlight);
    while (!glfwWindowShouldClose(context.Get<PresentFeature>().window->pWindow)) {
        glfwPollEvents();
        DrawFrame(
            context, 
            resources
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