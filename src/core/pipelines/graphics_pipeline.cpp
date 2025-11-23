#include <graphics_pipeline.h>

GraphicsPipelineBuilder::GraphicsPipelineBuilder(RenderContext& context) {
    this->context = &context;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::AddShaderStage(ShaderStage stage, ShaderBinary& binary) {
    VkShaderModule vkModule;
    VkShaderModuleCreateInfo moduleInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    moduleInfo.codeSize = binary.size_in_bytes();
    moduleInfo.pCode = binary.spirVWords.data();

    VK(vkCreateShaderModule(context->device->device, &moduleInfo, nullptr, &vkModule))

    shaderModules.push_back(vkModule);
    
    VkPipelineShaderStageCreateInfo stageInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stageInfo.stage = ToVkShaderStage(stage);
    stageInfo.module = vkModule;
    stageInfo.pName = "main";

    stages.push_back(stageInfo);

    return *this;
}