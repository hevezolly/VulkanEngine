#ifdef BLOCK
#include <glm/glm.hpp>
#include <common.h>
#include <vector>
#include <shader_stage.h>
#include <buffer.h>
#include <allocator_feature.h>
#include <render_context.h>

#ifndef BLOCK_NAME
#error "BLOCK_NAME must be defined"
#endif

#define BUFFER_INFO 0x1
#define IMAGE_INFO 0x2
#define TEXEL_BUFFER_VIEW_INFO 0x4
#define INCLUDES_SAMPLER 0x8
#define INCLUDES_IMAGE 0x10

struct BLOCK_NAME {

#define DEFINITION
#define WRAPPER(t, n, b, s, dc, dt, bv, iv, sv, info) t n;
#include "define_shader_bindings.h"
BLOCK
#undef WRAPPER
#undef DEFINITION

    static constexpr uint32_t size() {
        uint32_t value = 0;
        #define WRAPPER(t, n, b, s, dc, dt, bv, iv, sv, info) value++;
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER
        return value;
    }

    static constexpr uint32_t count(VkDescriptorType type) {
        uint32_t value = 0;
        #define WRAPPER(t, n, b, s, dc, dt, bv, iv, sv, info) value += (dt == type ? 1 : 0);
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER
        return value;
    }

    static MemChunk<VkDescriptorSetLayoutBinding> FillLayoutBindings(RenderContext& context) {
        auto chunk = context.Get<Allocator>().BumpAllocate<VkDescriptorSetLayoutBinding>(size());
        
        // ToVkShaderStage(s), nullptr};
        uint32_t counter = 0;
        #define WRAPPER(t, n, b, s, dc, dt, bv, iv, sv, info) chunk[counter++].binding = b;  
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER

        counter = 0;
        #define WRAPPER(t, n, b, s, dc, dt, bv, iv, sv, info) chunk[counter++].descriptorType = dt;  
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER

        counter = 0;
        #define WRAPPER(t, n, b, s, dc, dt, bv, iv, sv, info) chunk[counter++].descriptorCount = dc;  
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER

        counter = 0;
        #define WRAPPER(t, n, b, s, dc, dt, bv, iv, sv, info) chunk[counter++].stageFlags = ToVkShaderStage(s);  
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER

        counter = 0;
        #define WRAPPER(t, n, b, s, dc, dt, bv, iv, sv, info) chunk[counter++].pImmutableSamplers = nullptr;  
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER
        return chunk;
    }

    static MemChunk<VkDescriptorPoolSize> FillDescriptorSizes(RenderContext& context, uint32_t& actualCount) {
        auto chunk = context.Get<Allocator>().BumpAllocate<VkDescriptorPoolSize>(size());
        actualCount = 0;
        uint32_t index; 
        #define WRAPPER(t, n, b, s, dc, dt, bv, iv, sv, info) \
        index = UINT32_MAX; \
        for (uint32_t i = 0; i < actualCount; i++) { if (chunk[i].type == dt) {index = i; break;} } \
        if (index == UINT32_MAX) {index = actualCount++;} \
        chunk[index].type = dt; chunk[index].descriptorCount += dc;
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER
        return chunk;
    }

    MemChunk<VkWriteDescriptorSet> CollectDescriptorWrites(RenderContext& context, VkDescriptorSet set) {
        Allocator& allocator = context.Get<Allocator>();
        auto writes = allocator.BumpAllocate<VkWriteDescriptorSet>(size());
        uint32_t index;

        index = 0;
        #define WRAPPER(t, n, b, s, dc, dt, bv, iv, sv, info) writes[index++].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER

        index = 0;
        #define WRAPPER(t, n, b, s, dc, dt, bv, iv, sv, info) writes[index++].dstSet = set;
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER

        index = 0;
        #define WRAPPER(t, n, b, s, dc, dt, bv, iv, sv, info) writes[index++].dstBinding = b;
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER

        index = 0;
        #define WRAPPER(t, n, b, s, dc, dt, bv, iv, sv, info) writes[index++].descriptorType = dt;
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER

        index = 0;
        #define WRAPPER(t, n, b, s, dc, dt, bv, iv, sv, info) writes[index++].descriptorCount = dc;
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER

        index = 0;
        #define WRAPPER(t, n, b, s, dc, dt, bv, iv, sv, info) writes[index++].dstArrayElement = 0;
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER

        index = 0;
        #define WRAPPER(t, n, b, s, dc, dt, buffer_value, image_value, sampler_value, info) { \
            uint32_t i = index++; \
            if constexpr (((info) & BUFFER_INFO) == BUFFER_INFO) { \
                writes[i].pImageInfo = nullptr; \
                writes[i].pTexelBufferView = nullptr; \
                VkDescriptorBufferInfo* bufferInfo = allocator.BumpAllocate<VkDescriptorBufferInfo>(dc).data; \
                for (int __descriptor = 0; __descriptor < dc; __descriptor++) { \
                    bufferInfo[__descriptor].buffer = buffer_value; \
                    bufferInfo[__descriptor].offset = 0; \
                    bufferInfo[__descriptor].range = VK_WHOLE_SIZE; \
                } \
                writes[i].pBufferInfo = bufferInfo; \
            } else if constexpr (((info) & IMAGE_INFO) == IMAGE_INFO) { \
                writes[i].pTexelBufferView = nullptr; \
                writes[i].pBufferInfo = nullptr; \
                VkDescriptorImageInfo* imageInfo = allocator.BumpAllocate<VkDescriptorImageInfo>(dc).data; \
                for (int __descriptor = 0; __descriptor < dc; __descriptor++) { \
                    imageInfo[__descriptor].sampler = nullptr; \
                    imageInfo[__descriptor].imageView = nullptr; \
                    imageInfo[__descriptor].imageLayout = VK_IMAGE_LAYOUT_UNDEFINED; \
                    if constexpr (((info) & INCLUDES_SAMPLER) == INCLUDES_SAMPLER) imageInfo[__descriptor].sampler = sampler_value; \
                    if constexpr (((info) & INCLUDES_IMAGE) == INCLUDES_IMAGE) { \
                        imageInfo[__descriptor].imageView = image_value; \
                        imageInfo[__descriptor].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; \
                    } \
                } \
                writes[i].pImageInfo = imageInfo; \
            } \
        }
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER

        return writes;
    }

};

#include "define_shader_bindings.h"

#undef BUFFER_INFO
#undef IMAGE_INFO
#undef TEXEL_BUFFER_VIEW_INFO
#undef INCLUDES_SAMPLER
#undef INCLUDES_IMAGE

#undef BLOCK
#endif