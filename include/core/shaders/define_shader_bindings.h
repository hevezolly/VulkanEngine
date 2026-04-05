#ifdef UNIFORM_BUFFER
#undef UNIFORM_BUFFER
#undef SSBO
#undef DYNAMIC_SSBO
#undef DYNAMIC_UNIFORM
#undef IMAGE_STORAGE
#undef IMAGE
#undef IMAGE_SAMPLER
#undef SAMPLER
#endif

#ifdef WRAPPER

    #ifdef RESOURCES

        #define IMAGE_SAMPLER(name, binding, stage) WRAPPER(ImageSubresource, name##.image.id, binding, stage, true, false)
        #define IMAGE_STORAGE(name, binding, stage, access) WRAPPER(ImageSubresource, name##.image.id, binding, stage, (static_cast<unsigned int>(access & Access::Read) > 0), (static_cast<unsigned int>(access & Access::Write) > 0))
        #define IMAGE(name, binding, stage) WRAPPER(ImageSubresource, name##.image.id, binding, stage, true, false)
        #define SAMPLER(name, binding, stage)
        #define UNIFORM_BUFFER(name, binding, stage) WRAPPER(BufferRegion, name##.buffer.id, binding, stage, true, false)
        #define DYNAMIC_UNIFORM(name, binding, stage) WRAPPER(BufferRegion, name##.buffer.id, binding, stage, true, false)
        #define SSBO(name, binding, stage, access) WRAPPER(BufferRegion, name##.buffer.id, binding, stage, (static_cast<unsigned int>(access & Access::Read) > 0), (static_cast<unsigned int>(access & Access::Write) > 0))
        #define DYNAMIC_SSBO(name, binding, stage, access) WRAPPER(BufferRegion, name##.buffer.id, binding, stage, (static_cast<unsigned int>(access & Access::Read) > 0), (static_cast<unsigned int>(access & Access::Write) > 0))

    #elif defined(IMAGES) || defined(BUFFERS)

        #ifdef IMAGES

            #define IMAGE_SAMPLER(name, binding, stage) IMAGES(1, name, name##_sampler->vkSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, name##.image.id, name##_sampler.id)
            #define IMAGE(name, binding, stage) IMAGES(1, name, nullptr, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, name##.image.id, ResourceId())
            #define IMAGE_STORAGE(name, binding, stage, access) IMAGES(1, name, nullptr, VK_IMAGE_LAYOUT_GENERAL, name##.image.id, ResourceId())
            #define SAMPLER(name, binding, stage) IMAGES(1, nullptr, name##->vkSampler, VK_IMAGE_LAYOUT_UNDEFINED, name##.id, ResourceId())
            #ifndef BUFFERS
            #define SSBO(...)
            #define UNIFORM_BUFFER(...)
            #define DYNAMIC_UNIFORM(...)
            #define DYNAMIC_SSBO(...)
            #endif
            #endif

        #ifdef BUFFERS

            #ifndef IMAGES 
            #define IMAGE_SAMPLER(...)
            #define IMAGE_STORAGE(...)
            #define IMAGE(...)
            #define SAMPLER(...)
            #endif
            #define SSBO(name, binding, stage, access) BUFFERS(1, name##.buffer->vkBuffer, name##.offset, name##.size, name##.buffer.id, false)
            #define DYNAMIC_SSBO(name, binding, stage, access) BUFFERS(1, name##.buffer->vkBuffer, name##.offset, name##.size, name##.buffer.id, true)
            #define UNIFORM_BUFFER(name, binding, stage) BUFFERS(1, name##.buffer->vkBuffer, name##.offset, name##.size, name##.buffer.id, false)
            #define DYNAMIC_UNIFORM(name, binding, stage) BUFFERS(1, name##.buffer->vkBuffer, name##.offset, name##.size, name##.buffer.id, true)

        #endif
    #else

        #ifdef DEFINITION
            #define IMAGE_SAMPLER(name, binding, stage) \
            WRAPPER(ImageSubresource, name, binding, stage, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) \
            WRAPPER(ResourceRef<Sampler>, name##_sampler, binding, stage, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
            #else
            #define IMAGE_SAMPLER(name, binding, stage) WRAPPER(ImageSubresource, name, binding, stage, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
        #endif

        #define SSBO(name, binding, stage, access) WRAPPER(BufferRegion, name, binding, stage, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
        #define DYNAMIC_SSBO(name, binding, stage, access) WRAPPER(BufferRegion, name, binding, stage, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
        #define IMAGE(name, binding, stage) WRAPPER(ImageSubresource, name, binding, stage, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
        #define IMAGE_STORAGE(name, binding, stage, access) WRAPPER(ImageSubresource, name, binding, stage, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
        #define SAMPLER(name, binding, stage) WRAPPER(ResourceRef<Sampler>, name, binding, stage, 1, VK_DESCRIPTOR_TYPE_SAMPLER)
        #define UNIFORM_BUFFER(name, binding, stage) WRAPPER(BufferRegion, name, binding, stage, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
        #define DYNAMIC_UNIFORM(name, binding, stage) WRAPPER(BufferRegion, name, binding, stage, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)

    #endif
#endif