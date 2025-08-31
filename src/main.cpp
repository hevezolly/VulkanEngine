#include "volk.h"
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include <iostream>
#include <render_context.h>

int main() {
    
    {
        RenderContext context(800, 600, "VkEngine");

        while (!glfwWindowShouldClose(context.pWindow)) {
            glfwPollEvents();
        }
    }

    std::cout << "Success!" << std::endl;

    return 0;
}