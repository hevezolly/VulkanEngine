#pragma once

#include <volk.h>
#include <feature_set.h>
#include <common.h>

struct API DebuggingFeature: FeatureSet {

    VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo; 
    VkDebugUtilsMessengerEXT vkDebugMessenger;

    DebuggingFeature(RenderContext& context);
    virtual void PreInit();
    virtual void Destroy();
    virtual void GetRequiredExtentions(std::vector<const char*>& buffer);
    virtual void GetRequiredLayers(std::vector<const char*>& buffer);
};