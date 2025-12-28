#pragma once
#include <common.h>

struct API SamplerFilter {
    VkFilter minFilter;
    VkFilter magFilter;

    const static SamplerFilter NEAREST;
    const static SamplerFilter LINEAR;
};

struct API SamplerAddressMode {
    VkSamplerAddressMode uMode;
    VkSamplerAddressMode vMode;
    VkSamplerAddressMode wMode;

    const static SamplerAddressMode CLAMP;
    const static SamplerAddressMode REPEAT;
    const static SamplerAddressMode MIRROR_REPEAT;
};

struct API Sampler {
    VkSampler vkSampler;

    Sampler(VkSampler sampler, VkDevice device): 
        vkSampler(sampler), device(device) {}

    RULE_5(Sampler)

private:
    VkDevice device;
};