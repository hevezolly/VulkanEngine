#include <imgui_ui.h>
#include <render_context.h>
#include <graphics_feature.h>
#include <present_feature.h>

static void check_vk_result(VkResult err)
{
    VK(err)
}

#define BLOCK_NAME UiAttachments
#define BLOCK \
COLOR(color, LoadOp::Load)
#include <gen_attachments.h>


void ImguiUI::OnMessage(InitMsg*) {

    const uint32_t framesInFlight = 3;
    if (!context.Has<GraphicsFeature>()) {
        throw std::runtime_error("GraphicsFeature is required to use ImguiUI");
    }
    
    if (!context.Has<PresentFeature>()) {
        throw std::runtime_error("PresentFeature is required to use ImguiUI");
    }

    volkLoadInstance(context.vkInstance);
    volkLoadDevice(context.device());
    
    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;
    
    ImGui_ImplGlfw_InitForVulkan(context.Get<PresentFeature>().window->pWindow, true);
    _descriptorPool = new DescriptorPool(&context, {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE}
    }, true);
    
    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> attachmentRefs;
    UiAttachments::GetAttachmentDescriptions(attachments, 
        UiAttachments::Formats {
            context.Get<PresentFeature>().swapChain->format
        }
    );
    UiAttachments::GetColorAttachmentReferences(attachmentRefs);
    
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = attachmentRefs.size();
    subpass.pColorAttachments = attachmentRefs.data();
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    VkRenderPassCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = attachments.size();
    info.pAttachments = attachments.data();
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

    _waitFences = context.Get<Synchronization>().CreateFences(framesInFlight, true);
    readyToRender = false;
}

void ImguiUI::OnMessage(BeginFrameMsg* m) {
    currentFrame = m->inFlightFrame;

    if (readyToRender) {
        ImGui::EndFrame();
    }
    
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    readyToRender = true;
}

void ImguiUI::Record(ResourceRef<Image> output, GraphicsCommandBuffer& commandBuffer) {

    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();

    UiAttachments a;
    a.color = output;

    const FrameBuffer& frameBuffer = context.Get<GraphicsFeature>().CreateFrameBuffer(a, renderPass);    

    VkRenderPassBeginInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    renderPassInfo.renderPass = renderPass;

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = {frameBuffer.width, frameBuffer.height};

    renderPassInfo.framebuffer = frameBuffer.frameBuffer;

    vkCmdBeginRenderPass(commandBuffer.buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer.buffer);

    vkCmdEndRenderPass(commandBuffer.buffer);
}

void ImguiUI::OnMessage(PresentMsg* m) {

    assert(readyToRender);

    _waitFences[currentFrame]->Wait();
    _waitFences[currentFrame]->Reset();

    GraphicsCommandBuffer cmd = context.Get<CommandPool>().BorrowGraphicsBuffer();

    cmd.Begin();

    ResourceRef<Image> output = context.Get<PresentFeature>().swapChain->images[m->swapChainIndex];
    
    cmd.ImageBarrier(output, ResourceState{
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VkImageLayout::VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL
    });

    Record(output, cmd);
    
    cmd.ImageBarrier(output, ResourceState{
        VK_ACCESS_2_NONE,
        VK_PIPELINE_STAGE_2_NONE,
        VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    });
    cmd.End();

    Ref<Semaphore> startSemaphore = m->wait;
    Ref<Semaphore> endSemaphore = context.Get<Synchronization>().BorrowBinarySemaphore(true);
    m->wait = endSemaphore;


    context.Get<CommandPool>().Submit(
        cmd, 
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