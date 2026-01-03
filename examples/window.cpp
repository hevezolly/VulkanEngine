#include <iostream>
#include <render_context.h>
#include <shader_source.h>
#include <present_feature.h>
#include <graphics_feature.h>

void RunEngine() {
    
    WindowInitializer windowDescription{};
    SwapChainInitializer swapChainDescription{};
    windowDescription.width = 800;
    windowDescription.height = 600;
    windowDescription.hint = "VkEngine";
    std::cout << "before context" << std::endl;
    RenderContext context;
    context.WithFeature<PresentFeature>(windowDescription, swapChainDescription)
           .Initialize();
    std::cout << "after context" << std::endl;
    

    while (!glfwWindowShouldClose(context.Get<PresentFeature>().window->pWindow)) {
        glfwPollEvents();
        context.Send<BeginFrameMsg>({});
    }
}

int main() {
    
    try {
        RunEngine();
    } catch (const std::exception &e) {
        std::cout << e.what() << std::endl;
        throw;
    }

    std::cout << "Success!" << std::endl;

    return 0;
}