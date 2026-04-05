#pragma once
#ifndef VULKAN_ENGINE_NO_GLAM_HASH
#include <glm/glm.hpp>
#include <hash_combine.h>

namespace std {

    template<> struct hash<glm::vec2> {
        size_t operator()(const glm::vec2& value) const {
            size_t seed = 0;
            hash_combine(seed, value.x);
            hash_combine(seed, value.y);
            return seed;
        }
    };

    template<> struct hash<glm::vec3> {
        size_t operator()(const glm::vec3& value) const {
            size_t seed = 0;
            hash_combine(seed, value.x);
            hash_combine(seed, value.y);
            hash_combine(seed, value.z);
            return seed;
        }
    };

    template<> struct hash<glm::vec4> {
        size_t operator()(const glm::vec4& value) const {
            size_t seed = 0;
            hash_combine(seed, value.x);
            hash_combine(seed, value.y);
            hash_combine(seed, value.z);
            hash_combine(seed, value.w);
            return seed;
        }
    };

    template<> struct hash<glm::ivec2> {
        size_t operator()(const glm::ivec2& value) const {
            size_t seed = 0;
            hash_combine(seed, value.x);
            hash_combine(seed, value.y);
            return seed;
        }
    };

    template<> struct hash<glm::ivec3> {
        size_t operator()(const glm::ivec3& value) const {
            size_t seed = 0;
            hash_combine(seed, value.x);
            hash_combine(seed, value.y);
            hash_combine(seed, value.z);
            return seed;
        }
    };

    template<> struct hash<glm::ivec4> {
        size_t operator()(const glm::ivec4& value) const {
            size_t seed = 0;
            hash_combine(seed, value.x);
            hash_combine(seed, value.y);
            hash_combine(seed, value.z);
            hash_combine(seed, value.w);
            return seed;
        }
    };

    template<> struct hash<glm::uvec2> {
        size_t operator()(const glm::uvec2& value) const {
            size_t seed = 0;
            hash_combine(seed, value.x);
            hash_combine(seed, value.y);
            return seed;
        }
    };

    template<> struct hash<glm::uvec3> {
        size_t operator()(const glm::uvec3& value) const {
            size_t seed = 0;
            hash_combine(seed, value.x);
            hash_combine(seed, value.y);
            hash_combine(seed, value.z);
            return seed;
        }
    };

    template<> struct hash<glm::uvec4> {
        size_t operator()(const glm::uvec4& value) const {
            size_t seed = 0;
            hash_combine(seed, value.x);
            hash_combine(seed, value.y);
            hash_combine(seed, value.z);
            hash_combine(seed, value.w);
            return seed;
        }
    };

}

#endif