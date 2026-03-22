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
#include <graphics_node.h>
#include <render_graph.h>
#include <shader_loader.h>
#include <present_node.h>
#include <frame_dispatcher.h>
#include <dynamic_uniforms.h>

#define BLOCK_NAME Vertex
#define BLOCK \
VEC3(position, 0) \
VEC3(color, 1) \
VEC2(uv, 2)
#include <gen_vertex_data.h>

#define BLOCK_NAME ShaderInput
#define BLOCK \
DYNAMIC_UNIFORM(transforms, 0, Stage::Vertex) \
IMAGE_SAMPLER(img, 1, Stage::Fragment)
#include <gen_bindings.h>

#define BLOCK_NAME Attachments
#define BLOCK \
COLOR(color, LoadOp::Clear) \
DEPTH(depth, LoadOp::Clear) 
#include <gen_attachments.h>

struct UniformData {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    float time;
};

struct _Resources {
    Ref<GraphicsPipeline> pipeline;
    ResourceRef<Buffer> vertexBuffer;
    ResourceRef<Buffer> indexBuffer;
    ResourceRef<Image> image;
    ResourceRef<Image> image2;
    ResourceRef<Image> resourceImg;
    ResourceRef<Image> depth;
    ResourceRef<Image> depth2;
    ResourceRef<Image> depth3;
    ResourceRef<Sampler> sampler;
};

_Resources PrepareResources(
    RenderContext& context, 
    const uint32_t framesInFlight) {

    _Resources r{};
    
    ShaderBinary vertexBin = context.Get<ShaderLoader>().Get("shaders/basic.vert", Stage::Vertex);
    ShaderBinary fragmentBin = context.Get<ShaderLoader>().Get("shaders/basic.frag", Stage::Fragment);


    VkFormat dsFormat = context.Get<Device>().SelectSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);


    r.depth = context.Get<Resources>().CreateImage({dsFormat, 
        ImageUsage::DepthStencil, context.Get<PresentFeature>().swapChainExtent()});
    context.Get<Resources>().GiveName(r.depth, "depth");

    r.depth2 = context.Get<Resources>().CreateImage({dsFormat, 
        ImageUsage::DepthStencil, {100, 100}});
    context.Get<Resources>().GiveName(r.depth2, "depth2");

    r.depth3 = context.Get<Resources>().CreateImage({dsFormat, 
        ImageUsage::DepthStencil, {100, 100}});
    context.Get<Resources>().GiveName(r.depth3, "depth3");

    r.image = context.Get<Resources>().CreateImage({VK_FORMAT_B8G8R8A8_SRGB, 
        ImageUsage::ColorAttachment | ImageUsage::Sampled, {100, 100}});
    context.Get<Resources>().GiveName(r.image, "image");

    r.image2 = context.Get<Resources>().CreateImage({VK_FORMAT_B8G8R8A8_SRGB, 
        ImageUsage::ColorAttachment | ImageUsage::Sampled, {100, 100}});
    context.Get<Resources>().GiveName(r.image2, "image2");
    r.image2->clearValue.color = {{1.0f, 1.0f, 1.0f, 1.0f}};
    
    r.resourceImg = context.Get<Resources>().LoadImage(ImageUsage::Sampled, "test_img.png", VK_FORMAT_R8G8B8A8_SRGB);
    context.Get<Resources>().GiveName(r.resourceImg, "resourceImg");
    
    r.pipeline = context
        .Get<GraphicsFeature>().NewGraphicsPipeline()
        .AddVertex<Vertex>()
        .AddLayout<ShaderInput>()
        .SetAttachments<Attachments>({
            context.Get<PresentFeature>().swapChain->format,
            r.depth->description.format
        })
        .AddShaderStage(vertexBin)
        .AddShaderStage(fragmentBin)
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
    context.Get<Resources>().GiveName(r.vertexBuffer, "vertex_buffer");
    
    std::vector<uint16_t> indices = {
        0, 2, 1, 2, 0, 3,
        4, 6, 5, 6, 4, 7
    };
    r.indexBuffer = context.Get<Resources>().CreateBuffer<uint16_t>(BufferPreset::INDEX, indices);
    context.Get<Resources>().GiveName(r.indexBuffer, "index_buffer");

    r.sampler = context.Get<Resources>().CreateSampler(SamplerFilter::LINEAR, SamplerAddressMode::REPEAT);

    return r;
}

UniformData GetShaderData(RenderContext& context) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    SwapChain* swapChain = context.Get<PresentFeature>().swapChain;
    UniformData d{};
    d.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    d.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    d.proj = glm::perspective(glm::radians(25.0f), swapChain->extent.width / (float) swapChain->extent.height, 0.1f, 10.0f);
    
    d.proj[1][1] *= -1;
    d.time = time;
    return d;
} 

void DrawFrame(
    RenderContext& context,
    _Resources& r
) {
    context.BeginFrame();

    if (context.Get<PresentFeature>().SwapchainWasRecreated()) {
        r.depth = context.Get<Resources>().Resize(r.depth, context.Get<PresentFeature>().swapChainExtent());
    }

    ResourceRef<Image> outputImage = context.Get<PresentFeature>().AcquireNextImage();

    BufferRegion transforms = context.Get<DynamicUniforms>().Allocate(GetShaderData(context));  

    auto& node0 = context.Get<RenderGraph>().AddNode<GraphicsNode<Attachments, ShaderInput>>(r.pipeline);
    node0.SetName("draw1");
    Attachments attachments;
    attachments.color = r.image;
    attachments.depth = r.depth2;
    node0.SetAttachments(attachments);

    ShaderInput data{};
    data.transforms = transforms;
    data.img = r.resourceImg;
    data.img_sampler = r.sampler;
    node0.SetBindings(data);

    node0.SetIndexBuffer(r.indexBuffer, VkIndexType::VK_INDEX_TYPE_UINT16);
    node0.AddVertexBuffer(r.vertexBuffer);

    auto& node1 = context.Get<RenderGraph>().AddNode<GraphicsNode<Attachments, ShaderInput>>(r.pipeline);
    node1.SetName("draw2");
    attachments.color = r.image2;
    attachments.depth = r.depth3;
    node1.SetAttachments(attachments);

    data.transforms = transforms;
    data.img = r.image;
    data.img_sampler = r.sampler;
    node1.SetBindings(data);

    node1.SetIndexBuffer(r.indexBuffer, VkIndexType::VK_INDEX_TYPE_UINT16);
    node1.AddVertexBuffer(r.vertexBuffer);

    auto& node2 = context.Get<RenderGraph>().AddNode<GraphicsNode<Attachments, ShaderInput>>(r.pipeline);
    node2.SetName("draw3");

    attachments.color = outputImage;
    attachments.depth = r.depth;
    node2.SetAttachments(attachments);

    data.img = r.image2;
    node2.SetBindings(data);

    node2.SetIndexBuffer(r.indexBuffer, VkIndexType::VK_INDEX_TYPE_UINT16);
    node2.AddVertexBuffer(r.vertexBuffer);

    context.Get<RenderGraph>().AddNode<PresentNode>(outputImage).SetName("present");

    context.Get<RenderGraph>().Run();
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

    const uint32_t framesInFlight = 3;

    context.WithFeature<PresentFeature>(windowDescription, swapChainDescription)
           .WithFeature<FrameDispatcher>(framesInFlight)
           .WithFeature<GraphicsFeature>()
           .WithFeature<Registry>("examples/resources")
           .WithFeature<RenderGraph>()
           .WithFeature<DynamicUniforms>()
           .Initialize();
    volkLoadInstance(context.vkInstance);

    context.Get<Descriptors>().Preallocate<ShaderInput>(3);


    _Resources resources = PrepareResources(context, framesInFlight);
    glfwSwapInterval(1);
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