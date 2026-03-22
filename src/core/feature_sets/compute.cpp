#include <compute.h>
#include <render_context.h>

void Compute::OnMessage(CollectRequiredQueueTypesMsg* m) {
    m->requiredTypes |= QueueType::Compute;
}

ComputePipelineBuilder Compute::NewComputePipeline() {
    return ComputePipelineBuilder(context);
}

Ref<ComputePipeline> ComputePipelineBuilder::Build() {
    ASSERT(stages.size() == 1);
    
    VkPipeline pipeline;
    VkPipelineLayout layout = createLayout();

    VkComputePipelineCreateInfo createInfo{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    
    createInfo.layout = layout;
    createInfo.stage = stages[0];

    VK(vkCreateComputePipelines(context->device(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline))

    return context->New<ComputePipeline>(context->device(), pipeline, layout);
}