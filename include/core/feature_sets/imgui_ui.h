#pragma once
#include <feature_set.h>
#include "imgui.h"
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <descriptor_pool.h>
#include <present_feature.h>
#include <command_pool.h>

struct API ImguiUI: FeatureSet,
    CanHandle<InitMsg>,
    CanHandle<DestroyMsg>,
    CanHandle<BeginFrameMsg>,
    CanHandle<PresentMsg>
{
    using FeatureSet::FeatureSet;

    virtual void OnMessage(InitMsg*);
    virtual void OnMessage(DestroyMsg*);
    virtual void OnMessage(BeginFrameMsg*);
    virtual void OnMessage(PresentMsg*);

private:
    DescriptorPool* _descriptorPool;
    std::vector<GraphicsCommandBuffer> _commandBuffers;
    Refs<Fence> _waitFences;
    Refs<Semaphore> _presentReady;
    VkRenderPass renderPass;
    uint32_t currentFrame;
    bool readyToRender;
};