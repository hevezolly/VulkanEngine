#ifdef BLOCK
#include <common.h>
#include <glm/glm.hpp>
#include <vector>

#ifndef BLOCK_NAME
#error "BLOCK_NAME must be defined"
#endif

struct BLOCK_NAME {

#define WRAPPER(_t, _n, l, f) _t _n;
#include "define_scalar_atributes.h"
BLOCK
#undef WRAPPER

    static constexpr uint32_t size() {
        uint32_t __counter = 0;
#define WRAPPER(t, n, l, f) __counter++;
        #include "define_scalar_atributes.h"
BLOCK
#undef WRAPPER
        return __counter;
    }

    static void CollectAttributeDescription(std::vector<VkVertexInputAttributeDescription>& __attributes, uint32_t binding) {
        uint32_t initialSize = __attributes.size();
        __attributes.resize(initialSize + size());
        
        uint32_t __counter = initialSize;
#define WRAPPER(t, n, l, f) __attributes[__counter++].format = f;
        #include "define_scalar_atributes.h"
BLOCK
#undef WRAPPER

        __counter = initialSize;
#define WRAPPER(t, n, l, f) __attributes[__counter++].location = l; 
        #include "define_scalar_atributes.h"
BLOCK
#undef WRAPPER

        __counter = initialSize;
#define WRAPPER(t, n, l, f) __attributes[__counter++].offset = offsetof(BLOCK_NAME, n);
        #include "define_scalar_atributes.h"
BLOCK
#undef WRAPPER

        for (int i = initialSize; i < __attributes.size(); i++) {
            __attributes[i].binding = binding;
        }
    }
};

#undef BLOCK_NAME

#include "define_scalar_atributes.h"

#undef BLOCK
#endif