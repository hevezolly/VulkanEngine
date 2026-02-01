#pragma once

#include <volk.h>
#include <feature_set.h>
#include <common.h>
#include <messages.h>

struct API DebuggingFeature: FeatureSet, 
    CanHandle<EarlyInitMsg>,
    CanHandle<DestroyMsg>,
    CanHandle<CollectInstanceRequirementsMsg>
{

    VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo; 
    VkDebugUtilsMessengerEXT vkDebugMessenger;

    void NameVkObject(VkObjectType type, uint64_t handle, const std::string& name);

    DebuggingFeature(RenderContext& context);
    virtual void OnMessage(EarlyInitMsg*);
    virtual void OnMessage(DestroyMsg*);
    virtual void OnMessage(CollectInstanceRequirementsMsg*);
};