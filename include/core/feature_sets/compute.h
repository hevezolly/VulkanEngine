#pragma once

#include <feature_set.h>
#include <pipeline_builder.h>
#include <handles.h>

struct API ComputePipeline: Pipeline {
    using Pipeline::Pipeline;
};

struct API ComputePipelineBuilder: PipelineBuilder<ComputePipelineBuilder> {
    
    Ref<ComputePipeline> Build();

    friend struct Compute;
private: 
    ComputePipelineBuilder(RenderContext& c): PipelineBuilder(c) {};

protected:
    ComputePipelineBuilder& getSelf() {
        return *this;
    }
};

struct API Compute: FeatureSet,
    CanHandle<CollectRequiredQueueTypesMsg>
{
    using FeatureSet::FeatureSet;

    ComputePipelineBuilder NewComputePipeline();

    void OnMessage(CollectRequiredQueueTypesMsg*);

};