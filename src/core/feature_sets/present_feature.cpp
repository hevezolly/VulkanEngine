#include <present_feature.h>
#include <render_context.h>

PresentFeature::PresentFeature(
    RenderContext& context,
    const WindowInitializer& windowArgs, 
    const SwapChainInitializer& swapChainArgs): 
FeatureSet(context), 
swapChainFrameBuffers(context)
{
    this->windowArgs = windowArgs;
    this->swapChainArgs = swapChainArgs;

    if (!glfwInit()) {
        std::cout << "Failed to init glfw" << std::endl;
        std::exit(1);
    }
}

void PresentFeature::GetRequiredExtentions(std::vector<const char*>& buffer) {
    uint32_t extCount = 0;
    const char** exts = glfwGetRequiredInstanceExtensions(&extCount);
    if (!exts || extCount == 0) 
    {
        std::cout << "glfwGetRequiredInstanceExtensions failed" << std::endl;
        std::exit(1);
    }

    for (int i = 0; i < extCount; i++) {
        buffer.push_back(exts[i]);
    }
}

void PresentFeature::PreInit() {
    window = new Window(context.vkInstance, windowArgs);
}

void PresentFeature::Init() {
    swapChain= new SwapChain(&context, swapChainArgs);
}

void PresentFeature::Destroy() {
    delete swapChain;
    delete window;
    
    glfwTerminate();
}

Ref<FrameBuffer> PresentFeature::GetFrameBuffer(uint32_t swapChainImage, VkRenderPass renderPass) {
    Ref<FrameBuffer> result = swapChainFrameBuffers.Get(
        &context, 
        swapChain->images[static_cast<size_t>(swapChainImage)].view,
        renderPass
    );
    LOG("get swap chain frame buffer. size: " << swapChainFrameBuffers.size())
    return result;
}

uint32_t PresentFeature::swapChainSize() {
    return swapChain->images.size();
}

void PresentFeature::recreateSwapChain() {

    int width = 0, height = 0;
    glfwGetFramebufferSize(window->pWindow, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window->pWindow, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(context.device());
    swapChainFrameBuffers.Clear();
    delete swapChain;
    swapChain = new SwapChain(&context, swapChainArgs);
}

uint32_t PresentFeature::AcquireNextImage(Ref<Semaphore> imageReady) {
    uint32_t nextImage = 0;
    VkResult result = vkAcquireNextImageKHR(context.device(), swapChain->swapChain, UINT64_MAX, imageReady->vk, VK_NULL_HANDLE, &nextImage);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return AcquireNextImage(imageReady);
    }
    else {
        VK(result)
    }

    return nextImage;
}

void PresentFeature::Present(uint32_t swapChainImageIndex, Ref<Semaphore> wait) {
    VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &wait->vk;

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain->swapChain;
    presentInfo.pImageIndices = &swapChainImageIndex;

    VkResult result = vkQueuePresentKHR(context.Get<Device>().queues.get(QueueType::Present), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateSwapChain();
    }
    else {
        VK(result)
    }
}
