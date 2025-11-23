#pragma once
#include <vector>
#include <string>
#include <common.h>
#include <device.h>

enum struct API ShaderStage {
    Pixel,
    Vertex,
    Compute
};

struct API ShaderSource
{
    std::string name;
    ShaderStage stage;
    std::string source;
};

struct API ShaderBinary {
    std::vector<uint32_t> spirVWords;
    inline uint32_t size_in_bytes() const { return spirVWords.size() * sizeof(uint32_t); }
};

struct API ShaderCompiler {

    ShaderBinary FromSource(const ShaderSource& source);
};

struct API ShaderModule {
    VkShaderModule vkModule;

    ShaderModule(ShaderBinary binary, Device* device);
    ~ShaderModule();

    ShaderModule(const ShaderModule& other) = delete;
    ShaderModule& operator=(const ShaderModule& other) = delete;

    ShaderModule(ShaderModule&& other) noexcept;
    ShaderModule& operator=(ShaderModule&& other) noexcept;

private:
    void Clear();
    Device* device;
}
