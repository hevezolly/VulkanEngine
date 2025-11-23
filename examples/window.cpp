#include <iostream>
#include <render_context.h>
#include <shader_source.h>

void RunEngine() {
    RenderContextInitializer args;
    args.features = EngineFeatures::GraphicsPipeline |
                    EngineFeatures::WindowOutput;
    args.windowDescription.width = 800;
    args.windowDescription.height = 600;
    args.windowDescription.hint = "VkEngine";
    std::cout << "before context" << std::endl;
    RenderContext context(args);
    std::cout << "after context" << std::endl;

    ShaderSource vertexSource;
    vertexSource.name = "testVertex";
    vertexSource.stage = ShaderStage::Vertex;
    vertexSource.source = 
R"(#version 450
vec2 positions[3] = vec2[](
vec2(0.0, -0.5),
vec2(0.5, 0.5),
vec2(-0.5, 0.5)
);

void main() {
gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
})";

    ShaderCompiler compiler;
    ShaderBinary shaderBin = compiler.FromSource(vertexSource);

    std::cout << "shader compiled. size: " << shaderBin.spirVWords.size() << std::endl;

    while (!glfwWindowShouldClose(context.window->pWindow)) {
        glfwPollEvents();
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