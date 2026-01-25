#ifdef UNIFORM_BUFFER
#undef UNIFORM_BUFFER
#undef IMAGE
#undef IMAGE_SAMPLER
#undef SAMPLER
#endif

#ifdef WRAPPER

#ifdef RESOURCES

#define IMAGE_SAMPLER(name, binding, stage) WRAPPER(ResourceRef<Image>, name, binding, stage, true, false)
#define IMAGE(name, binding, stage) WRAPPER(ResourceRef<Image>, name, binding, stage, true, false)
#define SAMPLER(name, binding, stage)
#define UNIFORM_BUFFER(name, binding, stage) WRAPPER(ResourceRef<Buffer>, name, binding, stage, true, false)

#else

#ifdef DEFINITION
#define IMAGE_SAMPLER(name, binding, stage) \
WRAPPER(ResourceRef<Image>, name, binding, stage, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, nullptr, nullptr, IMAGE_INFO) \
WRAPPER(ResourceRef<Sampler>, name##_sampler, binding, stage, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, nullptr, nullptr, IMAGE_INFO)
#else
#define IMAGE_SAMPLER(name, binding, stage) WRAPPER(ResourceRef<Image>, name, binding, stage, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, name##->view->vkImageView, name##_sampler->vkSampler, IMAGE_INFO | INCLUDES_SAMPLER | INCLUDES_IMAGE)
#endif

#define IMAGE(name, binding, stage) WRAPPER(ResourceRef<Image>, name, binding, stage, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, nullptr, name##->view->vkImageView, nullptr, IMAGE_INFO | INCLUDES_IMAGE)
#define SAMPLER(name, binding, stage) WRAPPER(ResourceRef<Sampler>, name, binding, stage, 1, VK_DESCRIPTOR_TYPE_SAMPLER, nullptr, nullptr name##->vkSampler, IMAGE_INFO | INCLUDES_SAMPLER)
#define UNIFORM_BUFFER(name, binding, stage) WRAPPER(ResourceRef<Buffer>, name, binding, stage, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, name##->vkBuffer, nullptr, nullptr, BUFFER_INFO)

#endif
#endif