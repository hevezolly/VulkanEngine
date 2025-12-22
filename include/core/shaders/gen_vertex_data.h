#ifdef BLOCK
#include <glm/glm.hpp>
#include <common.h>
#include <vector>

#ifndef BLOCK_NAME
#error "BLOCK_NAME must be defined"
#endif

struct BLOCK_NAME {

#define DEFINITIONS
#include "define_scalar_atributes.h"
BLOCK

    static VkVertexInputBindingDescription GetBindingDescription() {

        VkVertexInputBindingDescription bindingDescription{};

        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(BLOCK_NAME);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;

    }

    static constexpr uint32_t size() {
        uint32_t __counter = 0;
        #define COUNT
        #include "define_scalar_atributes.h"
BLOCK
        return __counter;
    }

    static void CollectAttributeDescription(std::vector<VkVertexInputAttributeDescription>& __attributes) {
        uint32_t initialSize = __attributes.size();
        __attributes.resize(initialSize + size());
        
        uint32_t __counter = initialSize;
        #define FORMAT
        #include "define_scalar_atributes.h"
BLOCK

        __counter = initialSize;
        #define LOCATION
        #include "define_scalar_atributes.h"
BLOCK

        __counter = initialSize;
        #define OFFSET
        #include "define_scalar_atributes.h"
BLOCK

        for (int i = initialSize; i < __attributes.size(); i++) {
            __attributes[i].binding = 0;
        }
    }
};

#undef BLOCK_NAME

#include "define_scalar_atributes.h"

#undef BLOCK
#endif