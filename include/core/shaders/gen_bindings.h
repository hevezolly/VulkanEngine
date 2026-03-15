#ifdef BLOCK
#include <glm/glm.hpp>
#include <common.h>
#include <vector>
#include <image.h>
#include <shader_common.h>
#include <buffer.h>
#include <allocator_feature.h>
#include <render_context.h>
#include <resource_id.h>
#include <resource_storage.h>
#include <render_node.h>
#include <descriptor_identity.h>
#include <buffer_region.h>
#include <access_type.h>

#ifndef BLOCK_NAME
#error "BLOCK_NAME must be defined"
#endif

struct BLOCK_NAME {

#define DEFINITION
#define WRAPPER(t, n, b, s, dc, dt) t n;
#include "define_shader_bindings.h"
BLOCK
#undef WRAPPER
#undef DEFINITION

    static constexpr uint32_t size() {
        uint32_t value = 0;
        #define WRAPPER(t, n, b, s, dc, dt) value++;
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER
        return value;
    }

    static constexpr uint32_t size_resources() {
        uint32_t value = 0;

        #define DEFINITION
        #define WRAPPER(t, n, b, s, in, out) value++;
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER
        #undef DEFINITION
        return value;
    }

    static constexpr uint32_t size_inputs() {
        uint32_t value = 0;

        #define RESOURCES
        #define WRAPPER(t, n, b, s, in, out) if constexpr (in) value++;
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER
        #undef RESOURCES
        return value;
    }

    static constexpr uint32_t size_outputs() {
        uint32_t value = 0;

        #define RESOURCES
        #define WRAPPER(t, n, b, s, in, out) if constexpr (out) value++;
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER
        #undef RESOURCES
        return value;
    }

    static const constexpr uint32_t size_dynamic_states() {
        uint32_t value = 0;

        #define WRAPPER(...)
        #define BUFFERS(dc, buffer_value, offset, size, buffer_id, dynamic) if constexpr (dynamic) {value++;}
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER
        #undef BUFFERS
        return value;
    }

    void write_inputs(NodeDependency* dependencies) const {
        uint32_t index = 0;

        #define RESOURCES
        #define WRAPPER(t, id, b, s, in, out) \
        if constexpr (in) { \
        dependencies[index].resource = id; \
        dependencies[index++].state = ResourceState {VK_ACCESS_2_SHADER_READ_BIT, ToVkPipelineStage(s), VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};}
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER
        #undef RESOURCES
    }

    void write_outputs(NodeDependency* dependencies) const {
        uint32_t index = 0;

        #define RESOURCES
        #define WRAPPER(t, id, b, s, in, out) \
        if constexpr (out) { \
        dependencies[index].resource = id; \
        dependencies[index++].state = ResourceState { \
            VK_ACCESS_2_SHADER_WRITE_BIT | (in ? VK_ACCESS_2_SHADER_READ_BIT : 0), \
            ToVkPipelineStage(s), \
            VkImageLayout::VK_IMAGE_LAYOUT_GENERAL};}
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER
        #undef RESOURCES
    }

    static constexpr uint32_t count(VkDescriptorType type) {
        uint32_t value = 0;
        #define WRAPPER(t, n, b, s, dc, dt) value += (dt == type ? 1 : 0);
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER
        return value;
    }

    static MemChunk<VkDescriptorSetLayoutBinding> FillLayoutBindings(RenderContext& context) {
        auto chunk = context.Get<Allocator>().BumpAllocate<VkDescriptorSetLayoutBinding>(size());
        
        // ToVkShaderStage(s), nullptr};
        uint32_t counter = 0;
        #define WRAPPER(t, n, b, s, dc, dt) chunk[counter++].binding = b;  
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER

        counter = 0;
        #define WRAPPER(t, n, b, s, dc, dt) chunk[counter++].descriptorType = dt;  
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER

        counter = 0;
        #define WRAPPER(t, n, b, s, dc, dt) chunk[counter++].descriptorCount = dc;  
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER

        counter = 0;
        #define WRAPPER(t, n, b, s, dc, dt) chunk[counter++].stageFlags = ToVkShaderStage(s);  
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER

        counter = 0;
        #define WRAPPER(t, n, b, s, dc, dt) chunk[counter++].pImmutableSamplers = nullptr;  
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER
        return chunk;
    }

    static MemChunk<VkDescriptorPoolSize> FillDescriptorSizes(RenderContext& context, uint32_t& actualCount) {
        auto chunk = context.Get<Allocator>().BumpAllocate<VkDescriptorPoolSize>(size());
        actualCount = 0;
        uint32_t index; 
        #define WRAPPER(t, n, b, s, dc, dt) \
        index = UINT32_MAX; \
        for (uint32_t i = 0; i < actualCount; i++) { if (chunk[i].type == dt) {index = i; break;} } \
        if (index == UINT32_MAX) {index = actualCount++;} \
        chunk[index].type = dt; chunk[index].descriptorCount += dc;
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER
        return chunk;
    }

    void FillDescriptorSetIdentity(DescriptorSetIdentity& setId) const {
        #define WRAPPER(...)
        #define IMAGES(dc, image_value, sampler_value, image_layout, main_id, second_id) { \
            DescriptorIdentity _id; \
            _id.resource = main_id; \
            _id.additional.sampler = second_id; \
            setId.add(_id); \
        }
        #define BUFFERS(dc, buffer_value, offset, size, buffer_id, dynamic) { \
            DescriptorIdentity _id; \
            _id.resource = buffer_id; \
            _id.additional.bufferRange = {dynamic ? 0 : offset, size}; \
            setId.add(_id); \
        }
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER
        #undef IMAGES
        #undef BUFFERS
    }

    void FillDynamicState(uint32_t* dynamicState) const {
        uint32_t dynamicStateIndex = 0;
        #define WRAPPER(...)
        #define BUFFERS(dc, buffer_value, _offset, size, buffer_id, dynamic) { \
            if constexpr (dynamic) { \
                dynamicState[dynamicStateIndex++] = _offset; \
            } \
        }
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER
        #undef BUFFERS
    }

    MemChunk<VkWriteDescriptorSet> CollectDescriptorWrites(RenderContext& context, VkDescriptorSet set) const {
        Allocator& allocator = context.Get<Allocator>();
        auto writes = allocator.BumpAllocate<VkWriteDescriptorSet>(size());
        uint32_t index;

        index = 0;
        #define WRAPPER(t, n, b, s, dc, dt) writes[index++].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER

        index = 0;
        #define WRAPPER(t, n, b, s, dc, dt) writes[index++].dstSet = set;
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER

        index = 0;
        #define WRAPPER(t, n, b, s, dc, dt) writes[index++].dstBinding = b;
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER

        index = 0;
        #define WRAPPER(t, n, b, s, dc, dt) writes[index++].descriptorType = dt;
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER

        index = 0;
        #define WRAPPER(t, n, b, s, dc, dt) writes[index++].descriptorCount = dc;
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER

        index = 0;
        #define WRAPPER(t, n, b, s, dc, dt) writes[index++].dstArrayElement = 0;
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER

        index = 0;
        #define WRAPPER(...)
        #define IMAGES(dc, image_value, sampler_value, image_layout, image_id, sampler_id) { \
            uint32_t i = index++; \
            writes[i].pTexelBufferView = nullptr; \
            writes[i].pBufferInfo = nullptr; \
            VkDescriptorImageInfo* imageInfo = allocator.BumpAllocate<VkDescriptorImageInfo>(dc).data; \
            for (int __descriptor = 0; __descriptor < dc; __descriptor++) { \
                imageInfo[__descriptor].sampler = sampler_value; \
                imageInfo[__descriptor].imageView = image_value; \
                imageInfo[__descriptor].imageLayout = image_layout; \
            } \
            writes[i].pImageInfo = imageInfo; \
        }
        #define BUFFERS(dc, buffer_value, _offset, size, buffer_id, dynamic) { \
            uint32_t i = index++; \
            writes[i].pImageInfo = nullptr; \
            writes[i].pTexelBufferView = nullptr; \
            VkDescriptorBufferInfo* bufferInfo = allocator.BumpAllocate<VkDescriptorBufferInfo>(dc).data; \
            for (int __descriptor = 0; __descriptor < dc; __descriptor++) { \
                bufferInfo[__descriptor].buffer = buffer_value; \
                bufferInfo[__descriptor].offset = dynamic ? 0 : _offset; \
                bufferInfo[__descriptor].range = size; \
            } \
            writes[i].pBufferInfo = bufferInfo; \
        }
        #include "define_shader_bindings.h"
        BLOCK
        #undef WRAPPER
        #undef IMAGES
        #undef BUFFERS

        return writes;
    }

};

#include "define_shader_bindings.h"

#undef BLOCK
#undef BLOCK_NAME
#endif