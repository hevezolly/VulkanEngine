#include <iostream>
#include <render_context.h>
#include <shader_source.h>
#include <present_feature.h>
#include <graphics_feature.h>
#include <command_pool.h>
#include "raw_draw.h"

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
    LOG("init")

    const uint32_t framesInFlight = 3;
    Resources resources = PrepareResources(context, framesInFlight);

    LOG("command buffer built")

    uint32_t currentFrame = 0;
    while (!glfwWindowShouldClose(context.Get<PresentFeature>().window->pWindow)) {
        glfwPollEvents();
        LOG("poll events")
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