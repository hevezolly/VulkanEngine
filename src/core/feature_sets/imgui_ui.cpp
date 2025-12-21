#include <imgui_ui.h>
#include <render_context.h>
#include <graphics_feature.h>
#include <present_feature.h>

static void check_vk_result(VkResult err)
{
    VK(err)
}

void ImguiUI::OnMessage(InitMsg*) {

    const uint32_t framesInFlight = 3;
    if (!context.Has<GraphicsFeature>()) {
        throw std::runtime_error("GraphicsFeature is required to use ImguiUI");
    }

    if (!context.Has<PresentFeature>()) {
        throw std::runtime_error("PresentFeature is required to use ImguiUI");
    }

    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;

    ImGui_ImplGlfw_InitForVulkan(context.Get<PresentFeature>().window->pWindow, true);
    LOG(7)
    _descriptorPool = new DescriptorPool(&context, {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE}
    }, true);

    VkAttachmentDescription attachment = {};
    attachment.format = context.Get<PresentFeature>().swapChain->format;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference color_attachment = {};
    color_attachment.attachment = 0;
    color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment;
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    VkRenderPassCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = 1;
    info.pAttachments = &attachment;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = 1;
    info.pDependencies = &dependency;
    VK(vkCreateRenderPass(context.device(), &info, nullptr, &renderPass));

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.ApiVersion = context.apiVersion;              // Pass in your value of VkApplicationInfo::apiVersion, otherwise will default to header version.
    init_info.Instance = context.vkInstance;
    init_info.PhysicalDevice = context.Get<Device>().vkPhysicalDevice;
    init_info.Device = context.device();
    init_info.QueueFamily = context.Get<Device>().queueFamilies.get(QueueType::Graphics);
    init_info.Queue = context.Get<Device>().queues.get(QueueType::Graphics);
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = _descriptorPool->vkPool;
    init_info.MinImageCount = framesInFlight;
    init_info.ImageCount = framesInFlight;
    init_info.Allocator = nullptr;
    init_info.PipelineInfoMain.RenderPass = renderPass;
    init_info.PipelineInfoMain.Subpass = 0;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info);

    _commandBuffers.reserve(framesInFlight);

    for (int i = 0; i < framesInFlight; i++) {
        _commandBuffers.push_back(context.Get<CommandPool>().CreateGraphicsBuffer());
    }

    _waitFences = context.Get<Synchronization>().CreateFences(framesInFlight, true);
    _presentReady = context.Get<Synchronization>().CreateSemaphores(context.Get<PresentFeature>().swapChainSize());
    readyToRender = false;
}

void ImguiUI::OnMessage(BeginFrameMsg* m) {
    currentFrame = m->inFlightFrame;
    
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    readyToRender = true;
}

void ImguiUI::OnMessage(PresentMsg* m) {

    assert(readyToRender);

    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();

    _waitFences[currentFrame]->Wait();
    _waitFences[currentFrame]->Reset();

    _commandBuffers[currentFrame].Reset();
    _commandBuffers[currentFrame].Begin();

    Ref<FrameBuffer> frameBuffer = context.Get<PresentFeature>().GetFrameBuffer(m->swapChainIndex, renderPass);

    VkRenderPassBeginInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    renderPassInfo.renderPass = renderPass;

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = {frameBuffer->width, frameBuffer->height};

    renderPassInfo.framebuffer = frameBuffer->frameBuffer;

    vkCmdBeginRenderPass(_commandBuffers[currentFrame].buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    ImGui_ImplVulkan_RenderDrawData(draw_data, _commandBuffers[currentFrame].buffer);

    _commandBuffers[currentFrame].EndRenderPass();
    _commandBuffers[currentFrame].End();

    Ref<Semaphore> startSemaphore = m->wait;
    Ref<Semaphore> endSemaphore = _presentReady[m->swapChainIndex];
    m->wait = endSemaphore;

    context.Get<CommandPool>().Submit(
        _commandBuffers[currentFrame], 
        {{startSemaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}}, 
        {endSemaphore}, 
        _waitFences[currentFrame]
    );

    readyToRender = false;
}   

void ImguiUI::OnMessage(DestroyMsg*) {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    delete _descriptorPool;
    vkDestroyRenderPass(context.device(), renderPass, nullptr);
}