#pragma once
#include <volk.h>
#include <vector>
#include <string>
#include <common.h>
#include <shader_common.h>


struct API ShaderSource
{
    std::string name;
    Stage stage;
    std::string source;
};

struct API ShaderBinary {
    std::vector<uint32_t> spirVWords;
    inline uint32_t size_in_bytes() const { return spirVWords.size() * sizeof(uint32_t); }
};

struct API ShaderCompiler {

    ShaderBinary FromSource(const ShaderSource& source);
};
