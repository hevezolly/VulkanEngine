#pragma once
#include <volk.h>
#include <vector>
#include <string>
#include <common.h>

enum struct API ShaderStage {
    Pixel,
    Vertex,
    Compute
};

VkShaderStageFlagBits ToVkShaderStage(ShaderStage stage);


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
