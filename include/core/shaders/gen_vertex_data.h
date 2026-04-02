#ifdef BLOCK
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <common.h>
#include <glm/glm.hpp>
#include <vector>
#include <hash_combine.h>

#ifndef BLOCK_NAME
#error "BLOCK_NAME must be defined"
#endif

struct BLOCK_NAME {

#define WRAPPER(_t, _n, l, f) _t _n;
#include "define_scalar_atributes.h"
BLOCK
#undef WRAPPER

    bool operator==(const BLOCK_NAME& other) const {
        bool result = true;

#define WRAPPER(t, n, l, f) result &= n == other.##n;
        #include "define_scalar_atributes.h"
BLOCK
#undef WRAPPER

        return result;
    }

    static constexpr uint32_t size() {
        uint32_t __counter = 0;
#define WRAPPER(t, n, l, f) __counter++;
        #include "define_scalar_atributes.h"
BLOCK
#undef WRAPPER
        return __counter;
    }

    static void CollectAttributeDescription(std::vector<VkVertexInputAttributeDescription>& __attributes, uint32_t binding, uint32_t& location) {
        uint32_t initialSize = __attributes.size();
        __attributes.resize(initialSize + size());
        
        uint32_t __counter = initialSize;
#define WRAPPER(t, n, l, f) __attributes[__counter++].format = f;
        #include "define_scalar_atributes.h"
BLOCK
#undef WRAPPER

        __counter = initialSize;
#define WRAPPER(t, n, l, f) location += l; __attributes[__counter++].location = location; 
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

namespace std {
    template<> struct hash<BLOCK_NAME> {
        
        size_t operator()(const BLOCK_NAME & value) const {
                size_t seed = 0;
#define WRAPPER(t, n, l, f) hash_combine(seed, value.##n);
        #include "define_scalar_atributes.h"
BLOCK
#undef WRAPPER
            return seed;
        }
    };
}

#undef BLOCK_NAME

#include "define_scalar_atributes.h"

#undef BLOCK
#endif