#include <graphics_pipeline.h>

GraphicsPipelineBuilder::GraphicsPipelineBuilder(RenderContext& context) {
    this->context = &context;
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization.depthClampEnable = VK_FALSE;
    rasterization.depthBiasEnable = VK_FALSE;
    rasterization.lineWidth = 1.f;
    rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::AddShaderStage(
    ShaderStage stage, 
    ShaderBinary& binary
) {
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

GraphicsPipelineBuilder& GraphicsPipelineBuilder::AddDynamicState(VkDynamicState state) {
    dynamicStates.push_back(state);
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetViewport(VkViewport viewport) {
    this->viewport = viewport;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetScissor(VkRect2D scissor) {
    this->scissor = scissor;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetTopology(
    VkPrimitiveTopology topology, 
    bool restartEnabled=false
) {
    inputAssembly.topology = topology;
    inputAssembly.primitiveRestartEnable = restartEnabled ? VK_TRUE : VK_FALSE;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetCullMode(
    VkCullModeFlagBits mode, 
    VkFrontFace frontFace = VK_FRONT_FACE_CLOCKWISE
) {
    rasterization.cullMode = mode;
    rasterization.frontFace = frontFace;
    return *this;
}