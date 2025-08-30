#include "volk.h"
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

#include <iostream>

static void die(const char* msg) {
    std::cout << "Fatal: " << msg << std::endl;
    std::exit(1);
}

int main() {
    if (!glfwInit())
        die("failed to init glfw");

    glfwTerminate();

    std::cout << "Success!" << std::endl;

    return 0;
}