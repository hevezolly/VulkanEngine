#include "volk.h"
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include <iostream>
#include <render_context.h>

int main() {
    
    {
        RenderContextInitializer args;
        args.features = EngineFeatures::GraphicsPipeline |
                        EngineFeatures::WindowOutput;
        args.windowDescription.width = 800;
        args.windowDescription.height = 600;
        args.windowDescription.hint = "VkEngine";
        RenderContext context(args);

        while (!glfwWindowShouldClose(context.window->pWindow)) {
            glfwPollEvents();
        }
    }

    std::cout << "Success!" << std::endl;

    return 0;
}