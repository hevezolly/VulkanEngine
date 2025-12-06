#pragma once

#include <volk.h>
#include <feature_set.h>

struct DebuggingFeature: FeatureSet {

    VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo; 

    DebuggingFeature() {}
};