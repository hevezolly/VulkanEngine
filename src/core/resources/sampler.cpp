#include <sampler.h>
#include <render_context.h>
#include <resources.h>


const SamplerFilter SamplerFilter::LINEAR = {
    VkFilter::VK_FILTER_LINEAR,
    VkFilter::VK_FILTER_LINEAR
};

const SamplerFilter SamplerFilter::NEAREST = {
    VkFilter::VK_FILTER_NEAREST,
    VkFilter::VK_FILTER_NEAREST
};

const SamplerAddressMode SamplerAddressMode::CLAMP = {
    VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
};

const SamplerAddressMode SamplerAddressMode::REPEAT = {
    VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
    VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
    VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT
};

const SamplerAddressMode SamplerAddressMode::MIRROR_REPEAT = {
    VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
    VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
    VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT
};

Sampler::Sampler(VkSampler sampler, RenderContext& ctx):
vkSampler(sampler), context(&ctx) {

}

Sampler& Sampler::operator=(Sampler&& other) noexcept {
    if (this == &other)
        return *this;

    vkSampler = other.vkSampler;
    context = other.context;

    other.context = nullptr;
    other.vkSampler = VK_NULL_HANDLE;    

    return *this;
}

Sampler::Sampler(Sampler&& other) noexcept {
    *this = std::move(other);
}

Sampler::~Sampler() {
    if (context != nullptr) {
        vkDestroySampler(context->device(), vkSampler, nullptr);
        context = nullptr;
    }
}