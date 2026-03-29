#ifdef FLOAT
#undef FLOAT
#undef VEC2 
#undef VEC3 
#undef VEC4 
#undef INT 
#undef IVEC2
#undef IVEC3 
#undef IVEC4
#undef UINT 
#undef UVEC2
#undef UVEC3 
#undef UVEC4
#endif

#ifdef WRAPPER

#define FLOAT(name) WRAPPER(float, name, 1, VK_FORMAT_R32_SFLOAT) 
#define VEC2(name) WRAPPER(glm::vec2, name, 1, VK_FORMAT_R32G32_SFLOAT) 
#define VEC3(name) WRAPPER(glm::vec3, name, 1, VK_FORMAT_R32G32B32_SFLOAT) 
#define VEC4(name) WRAPPER(glm::vec4, name, 1, VK_FORMAT_R32G32B32A32_SFLOAT) 
#define INT(name) WRAPPER(int32_t, name, 1, VK_FORMAT_R32_SINT)
#define IVEC2(name) WRAPPER(glm::ivec2, name, 1, VK_FORMAT_R32G32_SINT)
#define IVEC3(name) WRAPPER(glm::ivec3, name, 1, VK_FORMAT_R32G32B32_SINT) 
#define IVEC4(name) WRAPPER(glm::ivec4, name, 1, VK_FORMAT_R32G32B32A32_SINT)
#define UINT(name) WRAPPER(uint32_t, name, 1, VK_FORMAT_R32_UINT) 
#define UVEC2(name) WRAPPER(glm::uvec2, name, 1, VK_FORMAT_R32G32_UINT)
#define UVEC3(name) WRAPPER(glm::uvec3, name, 1, VK_FORMAT_R32G32B32_UINT) 
#define UVEC4(name) WRAPPER(glm::uvec4, name, 1, VK_FORMAT_R32G32B32A32_UINT) 

#endif