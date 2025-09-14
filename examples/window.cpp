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
        std::cout << "before context" << std::endl;
        RenderContext context(args);
        std::cout << "after context" << std::endl;

        while (!glfwWindowShouldClose(context.window->pWindow)) {
            glfwPollEvents();
        }
    }

    std::cout << "Success!" << std::endl;

    return 0;
}