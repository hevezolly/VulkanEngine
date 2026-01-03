#include <shader_module.h>

ShaderModule::~ShaderModule() {
    if (device != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, vkModule, nullptr);
        device = VK_NULL_HANDLE;
    }
}

ShaderModule& ShaderModule::operator=(ShaderModule&& other) {
    if (this == &other)
        return *this;

    device = other.device;
    vkModule = other.vkModule;

    other.vkModule = VK_NULL_HANDLE;
    other.device = VK_NULL_HANDLE;

    return *this;
}

ShaderModule::ShaderModule(ShaderModule&& other) {
    *this = std::move(other);
}