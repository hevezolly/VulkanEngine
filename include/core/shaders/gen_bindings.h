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

#define BUFFER_INFO 0
#define IMAGE_INFO 1
#define TEXEL_BUFFER_VIEW_INFO 2

struct BLOCK_NAME {

#define WRAPPER(t, n, b, s, dc, dt, info) t n;
#include "define_shader_bindings.h"
BLOCK
#undef WRAPPER

    static constexpr uint32_t size() {
        uint32_t value = 0;
        #define WRAPPER(t, n, b, s, dc, dt, info) value++;
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER
        return value;
    }

    static constexpr uint32_t count(VkDescriptorType type) {
        uint32_t value = 0;
        #define WRAPPER(t, n, b, s, dc, dt, info) value += (dt == type ? 1 : 0);
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER
        return value;
    }

    static MemoryChunk<VkDescriptorSetLayoutBinding> FillLayoutBindings(RenderContext& context) {
        auto chunk = context.Get<Allocator>().Allocate<VkDescriptorSetLayoutBinding>(size());
        uint32_t counter = 0;
        #define WRAPPER(t, n, b, s, dc, dt, info) chunk[counter++] = {b, dt, dc, ToVkShaderStage(s), nullptr};  
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER
        return chunk;
    }

    static MemoryChunk<VkDescriptorPoolSize> FillDescriptorSizes(RenderContext& context, uint32_t& actualCount) {
        auto chunk = context.Get<Allocator>().Allocate<VkDescriptorPoolSize>(size());
        actualCount = 0;
        #define WRAPPER(t, n, b, s, dc, dt, info) \
        uint32_t index = UINT32_MAX; \
        for (uint32_t i = 0; i < actualCount; i++) { \
            if (chunk[i].type == t) {index = i; break;} \ 
        } \  
        if (index == UINT32_MAX) {index = actualCount++;} \
        chunk[index].type = t; chunk[index].descriptorCount += dc; \ 
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER
        return chunk;
    }

    MemoryChunk<VkWriteDescriptorSet> CollectDescriptorWrites(RenderContext& context, VkDescriptorSet set) {
        Allocator& allocator = context.Get<Allocator>();
        auto writes = allocator.Allocate<VkWriteDescriptorSet>(size());
        uint32_t idex;

        index = 0;
        #define WRAPPER(t, n, b, s, dc, dt, info) writes[index++].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER

        index = 0;
        #define WRAPPER(t, n, b, s, dc, dt, info) writes[index++].dstSet = set;
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER

        index = 0;
        #define WRAPPER(t, n, b, s, dc, dt, info) writes[index++].dstBinding = b;
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER

        index = 0;
        #define WRAPPER(t, n, b, s, dc, dt, info) writes[index++].descriptorType = dt;
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER

        index = 0;
        #define WRAPPER(t, n, b, s, dc, dt, info) writes[index++].descriptorCount = dc;
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER

        index = 0;
        #define WRAPPER(t, n, b, s, dc, dt, info) writes[index++].dstArrayElement = 0;
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER

        index = 0;
        #define WRAPPER(t, n, b, s, dc, dt, info) { \
            uint32_t i = index++; \
            if constexpr (info == BUFFER_INFO) { \
                writes[i].pImageInfo = nullptr; \
                writes[i].pTexelBufferView = nullptr; \
                VkDescriptorBufferInfo* bufferInfo = allocator.Allocate<VkDescriptorBufferInfo>(dc).data; \
                for (int descriptor = 0; descriptor < dc; descriptor++) { \
                    bufferInfo[descriptor].buffer = name[descriptor].vkBuffer; \
                    bufferInfo[descriptor].offset = 0; \
                    bufferInfo[descriptor].range = VK_WHOLE_SIZE; \
                } \
                writes[i].pBufferInfo = bufferInfo; \
            } else constexpr { \
                static_assert(false, "Not implemented") \
            } \
        }
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER
    }

};

#include "define_shader_bindings.h"

#undef BUFFER_INFO
#undef IMAGE_INFO
#undef TEXEL_BUFFER_VIEW_INFO

#undef BLOCK
#endif