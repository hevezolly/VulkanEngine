#include <graphics_feature.h>
#include <render_context.h>
#include <present_feature.h>

GraphicsPipelineBuilder::GraphicsPipelineBuilder(RenderContext& context):
    context(&context)
{
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;

    rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization.depthClampEnable = VK_FALSE;
    rasterization.depthBiasEnable = VK_FALSE;
    rasterization.lineWidth = 1.f;
    rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterization.polygonMode = VK_POLYGON_MODE_FILL;

    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    blending.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blending.blendEnable = VK_FALSE;
    blending.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    blending.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    blending.colorBlendOp = VK_BLEND_OP_ADD;
    blending.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blending.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blending.alphaBlendOp = VK_BLEND_OP_ADD;

    blendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blendingCreateInfo.logicOpEnable = VK_FALSE;
    blendingCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    blendingCreateInfo.attachmentCount = 1;
    blendingCreateInfo.pAttachments = &blending;
    blendingCreateInfo.blendConstants[0] = 0.0f;
    blendingCreateInfo.blendConstants[1] = 0.0f;
    blendingCreateInfo.blendConstants[2] = 0.0f;
    blendingCreateInfo.blendConstants[3] = 0.0f;

    pipelineLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayout.setLayoutCount = 0;
    pipelineLayout.pSetLayouts = nullptr;
    pipelineLayout.pushConstantRangeCount = 0;
    pipelineLayout.pPushConstantRanges = nullptr;

    colorAttachment.format = context.Get<PresentFeature>().swapChain->format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::AddShaderStage(
    ShaderStage stage, 
    ShaderBinary& binary
) {
    VkShaderModule vkModule;
    VkShaderModuleCreateInfo moduleInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    moduleInfo.codeSize = binary.size_in_bytes();
    moduleInfo.pCode = binary.spirVWords.data();

    VK(vkCreateShaderModule(context->device(), &moduleInfo, nullptr, &vkModule))

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
    bool restartEnabled
) {
    inputAssembly.topology = topology;
    inputAssembly.primitiveRestartEnable = restartEnabled ? VK_TRUE : VK_FALSE;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetCullMode(
    VkCullModeFlagBits mode, 
    VkFrontFace frontFace
) {
    rasterization.cullMode = mode;
    rasterization.frontFace = frontFace;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetDepthClampEnable(bool enable) {
    rasterization.depthClampEnable = enable ? VK_TRUE : VK_FALSE;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetPolygoneMode(VkPolygonMode mode) {
    rasterization.polygonMode = mode;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetColorBlending(BlendMethod& method) {
    blending.blendEnable = VK_TRUE;
    blending.colorBlendOp = method.op;
    blending.srcColorBlendFactor = method.src;
    blending.dstColorBlendFactor = method.dst;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetAlphaBlending(BlendMethod& method) {
    blending.blendEnable = VK_TRUE;
    blending.alphaBlendOp = method.op;
    blending.srcAlphaBlendFactor = method.src;
    blending.dstAlphaBlendFactor = method.dst;
    return *this;
}


Ref<GraphicsPipeline> GraphicsPipelineBuilder::Build() {
    GraphicsPipeline* pipeline = new GraphicsPipeline;
    pipeline->context = context;

    VK(vkCreatePipelineLayout(
        context->device(), 
        &pipelineLayout, 
        nullptr, 
        &pipeline->layout
    ));

    VkRenderPassCreateInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VK(vkCreateRenderPass(
        context->device(), 
        &renderPassInfo, 
        nullptr, 
        &pipeline->renderPass
    ));

    VkPipelineViewportStateCreateInfo viewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1;
    viewportState.pViewports = nullptr;
    viewportState.scissorCount = 1;
    viewportState.pScissors = nullptr;
    if (viewport.has_value())
        viewportState.pViewports = &viewport.value();
    if (scissor.has_value())
        viewportState.pScissors = &scissor.value();

    VkPipelineDynamicStateCreateInfo dynamicStateInfo{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamicStateInfo.dynamicStateCount = dynamicStates.size();
    dynamicStateInfo.pDynamicStates = dynamicStates.data();

    VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.stageCount = stages.size();
    pipelineInfo.pStages = stages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterization;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState = &blendingCreateInfo;
    pipelineInfo.pDynamicState = &dynamicStateInfo;
    pipelineInfo.layout = pipeline->layout;
    pipelineInfo.renderPass = pipeline->renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    VK(vkCreateGraphicsPipelines(
        context->device(), 
        VK_NULL_HANDLE,
        1, &pipelineInfo, nullptr, &pipeline->pipeline
    ));

    return context->Register(pipeline);
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::operator=(GraphicsPipelineBuilder&& other) noexcept {
    shaderModules = std::move(other.shaderModules);
    stages = std::move(other.stages);
    dynamicStates = std::move(other.dynamicStates);
    viewport = std::move(other.viewport);
    scissor = std::move(other.scissor);
    inputAssembly = other.inputAssembly;
    vertexInputInfo = other.vertexInputInfo;
    rasterization = other.rasterization;
    multisampling = other.multisampling;
    blending = other.blending;
    blendingCreateInfo = other.blendingCreateInfo;
    context = other.context;
    pipelineLayout = other.pipelineLayout;
    colorAttachment = other.colorAttachment;
    colorAttachmentRef = other.colorAttachmentRef;
    subpass = other.subpass;

    other.context = nullptr;

    return *this;
}

GraphicsPipelineBuilder::GraphicsPipelineBuilder(GraphicsPipelineBuilder&& other) noexcept {
    *this = std::move(other);
}

GraphicsPipelineBuilder::~GraphicsPipelineBuilder() {
    for (int i = 0; i < shaderModules.size(); i++) {
        vkDestroyShaderModule(context->device(), shaderModules[i], nullptr);
    }
    shaderModules.clear();
}

GraphicsPipeline::GraphicsPipeline() {}

GraphicsPipeline& GraphicsPipeline::operator=(GraphicsPipeline&& other) noexcept {
    if (&other == this)
        return *this;
    
    layout = other.layout;
    context = other.context;
    renderPass = other.renderPass;
    pipeline = other.pipeline;
    other.pipeline = VK_NULL_HANDLE;
    other.layout = VK_NULL_HANDLE;
    other.context = nullptr;
    other.renderPass = VK_NULL_HANDLE;

    return *this;
}

GraphicsPipeline::GraphicsPipeline(GraphicsPipeline&& other) noexcept {
    *this = std::move(other);
}

GraphicsPipeline::~GraphicsPipeline() {
    if (context == nullptr)
        return;
        
    vkDestroyPipeline(context->device(), pipeline, nullptr);
    vkDestroyPipelineLayout(context->device(), layout, nullptr);
    vkDestroyRenderPass(context->device(), renderPass, nullptr);
    context = nullptr;
}