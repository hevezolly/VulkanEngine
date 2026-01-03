#ifdef UNIFORM_BUFFER
#undef UNIFORM_BUFFER
#undef IMAGE
#undef IMAGE_SAMPLER
#undef SAMPLER
#endif

#ifdef WRAPPER

#ifdef DEFINITION
#define IMAGE_SAMPLER(name, binding, stage) \
WRAPPER(ImageView*, name, binding, stage, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, nullptr, nullptr, IMAGE_INFO) \
WRAPPER(Sampler*, name##_sampler, binding, stage, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, nullptr, nullptr, IMAGE_INFO)
#else
#define IMAGE_SAMPLER(name, binding, stage) WRAPPER(ImageView*, name, binding, stage, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, name##[__descriptor].vkImageView, name##_sampler[__descriptor].vkSampler, IMAGE_INFO | INCLUDES_SAMPLER | INCLUDES_IMAGE)
#endif

#define IMAGE(name, binding, stage) WRAPPER(ImageView*, name, binding, stage, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, nullptr, name##[__descriptor].vkImageView, nullptr, IMAGE_INFO | INCLUDES_IMAGE)
#define SAMPLER(name, binding, stage) WRAPPER(Sampler*, name, binding, stage, 1, VK_DESCRIPTOR_TYPE_SAMPLER, nullptr, nullptr name##[__descriptor].vkSampler, IMAGE_INFO | INCLUDES_SAMPLER)
#define UNIFORM_BUFFER(name, binding, stage) WRAPPER(Buffer*, name, binding, stage, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, name##[__descriptor].vkBuffer, nullptr, nullptr, BUFFER_INFO)

#endif