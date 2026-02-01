#include "core/shaders/shader_source.h"
#include <glslang_c_interface.h>
#include <stdexcept>
#include <glslang/Public/resource_limits_c.h>

glslang_stage_t GetShaderStage(Stage stage) {
    switch (stage)
    {
    case Stage::Compute:
        return GLSLANG_STAGE_COMPUTE;
    case Stage::Fragment:
        return GLSLANG_STAGE_FRAGMENT;
    case Stage::Vertex:
        return GLSLANG_STAGE_VERTEX;
    
    default:
        throw std::runtime_error("unknown shader stage");
    }
}

ShaderBinary ShaderCompiler::FromSource(const ShaderSource& source) 
{

    glslang_stage_t stage = GetShaderStage(source.stage);

    glslang_input_t input;
    input.language = GLSLANG_SOURCE_GLSL;
    input.stage = stage;
    input.client = GLSLANG_CLIENT_VULKAN;
    input.client_version = GLSLANG_TARGET_VULKAN_1_2;
    input.target_language = GLSLANG_TARGET_SPV;
    input.target_language_version = GLSLANG_TARGET_SPV_1_5;
    input.code = source.source.c_str();
    input.default_version = 100;
    input.default_profile = GLSLANG_NO_PROFILE;
    input.force_default_version_and_profile = false;
    input.forward_compatible = false;
    input.messages = GLSLANG_MSG_DEFAULT_BIT;
    input.resource = glslang_default_resource();

    glslang_shader_t* shader = glslang_shader_create(&input);

    ShaderBinary bin = {};
    bin.name = source.name;
    bin.stage = source.stage;
    

    if (!glslang_shader_preprocess(shader, &input))	{
        printf("GLSL preprocessing failed %s\n", source.name);
        printf("%s\n", glslang_shader_get_info_log(shader));
        printf("%s\n", glslang_shader_get_info_debug_log(shader));
        printf("%s\n", input.code);
        glslang_shader_delete(shader);
        return bin;
    }

    if (!glslang_shader_parse(shader, &input)) {
        printf("GLSL parsing failed %s\n", source.name);
        printf("%s\n", glslang_shader_get_info_log(shader));
        printf("%s\n", glslang_shader_get_info_debug_log(shader));
        printf("%s\n", glslang_shader_get_preprocessed_code(shader));
        glslang_shader_delete(shader);
        return bin;
    }

    glslang_program_t* program = glslang_program_create();
    glslang_program_add_shader(program, shader);

    if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
        printf("GLSL linking failed %s\n", source.name);
        printf("%s\n", glslang_program_get_info_log(program));
        printf("%s\n", glslang_program_get_info_debug_log(program));
        glslang_program_delete(program);
        glslang_shader_delete(shader);
        return bin;
    }

    glslang_program_SPIRV_generate(program, stage);

    bin.spirVWords.resize(glslang_program_SPIRV_get_size(program));
    glslang_program_SPIRV_get(program, bin.spirVWords.data());

    const char* spirv_messages = glslang_program_SPIRV_get_messages(program);
    if (spirv_messages)
        printf("(%s) %s\b", source.name, spirv_messages);

    glslang_program_delete(program);
    glslang_shader_delete(shader);

    return bin;
}
