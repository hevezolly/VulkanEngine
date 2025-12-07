#include <present_feature.h>
#include <render_context.h>

PresentFeature::PresentFeature(
    RenderContext& context,
    const WindowInitializer& windowArgs, 
    const SwapChainInitializer& swapChainArgs)
: FeatureSet(context)
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
