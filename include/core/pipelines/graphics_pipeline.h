#pragma once
#include <common.h>


struct API GraphicsPipeline {


    std::vector<VkShaderModule> shaders;
    

    GraphicsPipeline();

    ~GraphicsPipeline();

    GraphicsPipeline(const GraphicsPipeline& other) = delete;
    GraphicsPipeline& operator=(const GraphicsPipeline& other) = delete;

    GraphicsPipeline(GraphicsPipeline&& other) noexcept;
    GraphicsPipeline& operator=(GraphicsPipeline&& other) noexcept;
};