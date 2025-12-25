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

#define FLOAT(name, location) WRAPPER(float, name, location, VK_FORMAT_R32_SFLOAT) 
#define VEC2(name, location) WRAPPER(glm::vec2, name, location, VK_FORMAT_R32G32_SFLOAT) 
#define VEC3(name, location) WRAPPER(glm::vec3, name, location, VK_FORMAT_R32G32B32_SFLOAT) 
#define VEC4(name, location) WRAPPER(glm::vec4, name, location, VK_FORMAT_R32G32B32A32_SFLOAT) 
#define INT(name, location) WRAPPER(int32_t, name, location, VK_FORMAT_R32_SINT)
#define IVEC2(name, location) WRAPPER(glm::ivec2, name, location, VK_FORMAT_R32G32_SINT)
#define IVEC3(name, location) WRAPPER(glm::ivec3, name, location, VK_FORMAT_R32G32B32_SINT) 
#define IVEC4(name, location) WRAPPER(glm::ivec4, name, location, VK_FORMAT_R32G32B32A32_SINT)
#define UINT(name, location) WRAPPER(uint32_t, name, location, VK_FORMAT_R32_UINT) 
#define UVEC2(name, location) WRAPPER(glm::uvec2, name, location, VK_FORMAT_R32G32_UINT)
#define UVEC3(name, location) WRAPPER(glm::uvec3, name, location, VK_FORMAT_R32G32B32_UINT) 
#define UVEC4(name, location) WRAPPER(glm::uvec4, name, location, VK_FORMAT_R32G32B32A32_UINT) 

#endif