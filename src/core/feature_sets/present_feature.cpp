#include <present_feature.h>
#include <render_context.h>
#include <vk_collect.h>
#include <resources.h>

PresentFeature::PresentFeature(
    RenderContext& context,
    const WindowInitializer& windowArgs, 
    const SwapChainInitializer& swapChainArgs): 
FeatureSet(context)
{
    this->windowArgs = windowArgs;
    this->swapChainArgs = swapChainArgs;

    if (!glfwInit()) {
        std::cout << "Failed to init glfw" << std::endl;
        std::exit(1);
    }
}

void PresentFeature::OnMessage(CollectInstanceRequirementsMsg* m) {
    uint32_t extCount = 0;
    const char** exts = glfwGetRequiredInstanceExtensions(&extCount);
    if (!exts || extCount == 0) 
    {
        std::cout << "glfwGetRequiredInstanceExtensions failed" << std::endl;
        std::exit(1);
    }

    for (int i = 0; i < extCount; i++) {
        m->extentions->push_back(exts[i]);
    }
}

void PresentFeature::OnMessage(CollectDeviceRequirementsMsg* m) {
    m->extentions->push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

void PresentFeature::OnMessage(EarlyInitMsg*) {
    window = new Window(context.vkInstance, windowArgs);
}

void PresentFeature::OnMessage(InitMsg*) {
    swapChain= new SwapChain(&context, swapChainArgs);
}

void PresentFeature::OnMessage(DestroyMsg*) {
    delete swapChain;
    delete window;
    
    glfwTerminate();
}

uint32_t PresentFeature::swapChainSize() {
    return swapChain->images.size();
}

void UpdateSwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR vkSurface, SwapChainSupport& swapChainSupport) 
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, vkSurface, &swapChainSupport.capabilities);
    swapChainSupport.surfaceFormats = std::move(vkCollect<VkSurfaceFormatKHR>(
        vkGetPhysicalDeviceSurfaceFormatsKHR, device, vkSurface 
    ));

    swapChainSupport.presentModes = std::move(vkCollect<VkPresentModeKHR>(
        vkGetPhysicalDeviceSurfacePresentModesKHR, device, vkSurface
    ));
}

void PresentFeature::recreateSwapChain() {

    int width = 0, height = 0;
    glfwGetFramebufferSize(window->pWindow, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window->pWindow, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(context.device());
    delete swapChain;

    UpdateSwapChainSupport(context.Get<Device>().vkPhysicalDevice, window->vkSurface, swapChainSupport);

    swapChain = new SwapChain(&context, swapChainArgs);
}

uint32_t PresentFeature::AcquireNextImage(Ref<Semaphore> imageReady) {
    uint32_t nextImage = 0;
    VkResult result = vkAcquireNextImageKHR(context.device(), swapChain->swapChain, UINT64_MAX, imageReady->vk, VK_NULL_HANDLE, &nextImage);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateSwapChain();
        return AcquireNextImage(imageReady);
    }
    else {
        VK(result)
    }

    ResourceId img = swapChain->images[nextImage];
    context.Get<Resources>().SetSynchronizationContext(img, imageReady);

    return nextImage;
}

ResourceRef<Image> PresentFeature::AcquireNextImage() {

    Synchronization& sync = context.Get<Synchronization>();
    Ref<Semaphore> imageReady = sync.BorrowSemaphore();

    uint32_t nextImage = 0;
    VkResult result = vkAcquireNextImageKHR(context.device(), swapChain->swapChain, UINT64_MAX, imageReady->vk, VK_NULL_HANDLE, &nextImage);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        sync.ReturnSemaphorEarly(imageReady);
        recreateSwapChain();
        return AcquireNextImage();
    }
    else {
        VK(result)
    }

    ResourceRef<Image> img = swapChain->images[nextImage];
    context.Get<Resources>().SetSynchronizationContext(img, imageReady);

    return img;
}

void PresentFeature::OnMessage(PresentMsg* m) {

    uint32_t swapChainImageIndex = m->swapChainIndex;
    Ref<Semaphore> wait = m->wait;

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

void PresentFeature::OnMessage(CheckDeviceAppropriateMsg* m) {

    UpdateSwapChainSupport(m->device, window->vkSurface, swapChainSupport);

    bool supported = swapChainSupport.surfaceFormats.size() > 0 && swapChainSupport.presentModes.size() > 0;

    m->appropriate &= supported;
}

void PresentFeature::OnMessage(CollectRequiredQueueTypesMsg* m) {
    m->requiredTypes |= QueueType::Present;
}

VkExtent2D PresentFeature::swapChainExtent() {
    return {swapChain->images[0]->description.width, swapChain->images[0]->description.height};
}
