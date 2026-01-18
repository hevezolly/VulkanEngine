#pragma once
#include <common.h>
#include <resource_id.h>

struct RenderContext;

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

    Sampler(VkSampler sampler, RenderContext& ctx);

    RULE_5(Sampler)

private:
    RenderContext* context;
};