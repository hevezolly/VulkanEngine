#pragma once

#include <volk.h>
#include <feature_set.h>
#include <common.h>
#include <messages.h>

struct API DebuggingFeature: FeatureSet, CanHandle<EarlyInitMessage> {

    VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo; 
    VkDebugUtilsMessengerEXT vkDebugMessenger;

    DebuggingFeature(RenderContext& context);
    virtual void OnMessage(EarlyInitMessage*);
    virtual void Destroy();
    virtual void GetRequiredExtentions(std::vector<const char*>& buffer);
    virtual void GetRequiredLayers(std::vector<const char*>& buffer);
};