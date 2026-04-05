#include <graphics_feature.h>
#include <render_context.h>
#include <present_feature.h>
#include <shader_loader.h>

GraphicsPipelineBuilder::GraphicsPipelineBuilder(RenderContext& context):
    PipelineBuilder<GraphicsPipelineBuilder>(context), location(0)
{
    inputAssembly = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    rasterization = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterization.depthClampEnable = VK_FALSE;
    rasterization.depthBiasEnable = VK_FALSE;
    rasterization.lineWidth = 1.f;
    rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterization.polygonMode = VK_POLYGON_MODE_FILL;

    multisampling = {};
    multisampling = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    blending = {};
    blending.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blending.blendEnable = VK_FALSE;
    blending.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    blending.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    blending.colorBlendOp = VK_BLEND_OP_ADD;
    blending.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blending.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blending.alphaBlendOp = VK_BLEND_OP_ADD;

    blendingCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    blendingCreateInfo.logicOpEnable = VK_FALSE;
    blendingCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    blendingCreateInfo.attachmentCount = 1;
    blendingCreateInfo.pAttachments = &blending;
    blendingCreateInfo.blendConstants[0] = 0.0f;
    blendingCreateInfo.blendConstants[1] = 0.0f;
    blendingCreateInfo.blendConstants[2] = 0.0f;
    blendingCreateInfo.blendConstants[3] = 0.0f;

    dsState = std::nullopt;

    subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
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
    ASSERT(dsState.has_value() == dsRef.has_value());
    VkPipeline pipeline;
    VkRenderPass renderPass;
    VkPipelineLayout layout = createLayout();

    subpass.colorAttachmentCount = colorAttachmentRef.size();
    subpass.pColorAttachments = colorAttachmentRef.data();
    subpass.pDepthStencilAttachment = nullptr;
    if (dsRef.has_value())
        subpass.pDepthStencilAttachment = &dsRef.value();

    VkRenderPassCreateInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    renderPassInfo.attachmentCount = attachments.size();
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 0;
    renderPassInfo.pDependencies = nullptr;
    renderPassInfo.flags = 0;

    VK(vkCreateRenderPass(
        context->device(), 
        &renderPassInfo, 
        nullptr, 
        &renderPass
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

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInputInfo.vertexBindingDescriptionCount = vertexDescriptions.size();
    vertexInputInfo.pVertexBindingDescriptions = vertexDescriptions.data();
    vertexInputInfo.vertexAttributeDescriptionCount = vertexAttributes.size();
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttributes.data();

    VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.stageCount = stages.size();
    pipelineInfo.pStages = stages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterization;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;
    if (dsState.has_value())
        pipelineInfo.pDepthStencilState = &dsState.value();
    pipelineInfo.pColorBlendState = &blendingCreateInfo;
    pipelineInfo.pDynamicState = &dynamicStateInfo;
    pipelineInfo.layout = layout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;
    pipelineInfo.flags = 0;

    VK(vkCreateGraphicsPipelines(
        context->device(), 
        VK_NULL_HANDLE,
        1, &pipelineInfo, nullptr, &pipeline
    ));

    return context->New<GraphicsPipeline>(context->device(), pipeline, layout, renderPass);
}

GraphicsPipeline& GraphicsPipeline::operator=(GraphicsPipeline&& other) noexcept {
    if (&other == this)
        return *this;
    
    Pipeline::operator=(std::move(other));

    renderPass = other.renderPass;
    other.renderPass = VK_NULL_HANDLE;

    return *this;
}

GraphicsPipeline::GraphicsPipeline(GraphicsPipeline&& other) noexcept:
    Pipeline(std::move(other)) {
    renderPass = other.renderPass;
    other.renderPass = VK_NULL_HANDLE;
}

GraphicsPipeline::~GraphicsPipeline() {
    if (device == VK_NULL_HANDLE)
        return;
        
    vkDestroyRenderPass(device, renderPass, nullptr);
}

GraphicsPipelineBuilder GraphicsFeature::NewGraphicsPipeline() {
    return GraphicsPipelineBuilder(context);
}

void GraphicsFeature::OnMessage(CollectRequiredQueueTypesMsg* m){
    m->requiredTypes |= QueueType::Graphics;
}

void GraphicsFeature::OnMessage(BeginFrameMsg* m) {

    _frameBuffers.SetFrame(m->inFlightFrame);
    _frameBuffers->clear();
}

void GraphicsFeature::OnMessage(DestroyMsg*) {
    _frameBuffers.clear();
}