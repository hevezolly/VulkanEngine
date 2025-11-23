#pragma once
#include <vector>
#include <string>
#include <common.h>
#include <device.h>
#include <volk.h>

enum struct API ShaderStage {
    Pixel,
    Vertex,
    Compute
};

VkShaderStageFlagBits ToVkShaderStage(ShaderStage stage) {
    switch (stage) {
        case ShaderStage::Pixel:
            return VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
        case ShaderStage::Vertex:
            return VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
        case ShaderStage::Compute:
            return VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;
    } 
}


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
