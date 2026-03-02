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
#define UNIFORM_BUFFER(name, binding, stage) WRAPPER(BufferRegion, name, binding, stage, true, false)

#elif defined(IMAGES) || defined(BUFFERS)

#ifdef IMAGES

#define IMAGE_SAMPLER(name, binding, stage) IMAGES(1, name##->view->vkImageView, name##_sampler->vkSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, name##.id, name##_sampler.id)
#define IMAGE(name, binding, stage) IMAGES(1, name##->view->vkImageView, nullptr, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, name##.id, ResourceId())
#define SAMPLER(name, binding, stage) IMAGES(1, nullptr, name##->vkSampler, VK_IMAGE_LAYOUT_UNDEFINED, name##.id, ResourceId())
#ifndef BUFFERS
#define UNIFORM_BUFFER(...)
#endif
#endif

#ifdef BUFFERS

#ifndef IMAGES 
#define IMAGE_SAMPLER(...)
#define IMAGE(...)
#define SAMPLER(...)
#endif
#define UNIFORM_BUFFER(name, binding, stage) BUFFERS(1, name##.buffer->vkBuffer, name##.offset, name##.size, name##.buffer.id)

#endif
#else

#ifdef DEFINITION
#define IMAGE_SAMPLER(name, binding, stage) \
WRAPPER(ResourceRef<Image>, name, binding, stage, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) \
WRAPPER(ResourceRef<Sampler>, name##_sampler, binding, stage, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
#else
#define IMAGE_SAMPLER(name, binding, stage) WRAPPER(ResourceRef<Image>, name, binding, stage, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
#endif

#define IMAGE(name, binding, stage) WRAPPER(ResourceRef<Image>, name, binding, stage, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
#define SAMPLER(name, binding, stage) WRAPPER(ResourceRef<Sampler>, name, binding, stage, 1, VK_DESCRIPTOR_TYPE_SAMPLER)
#define UNIFORM_BUFFER(name, binding, stage) WRAPPER(BufferRegion, name, binding, stage, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)

#endif
#endif