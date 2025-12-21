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

#ifdef DEFINITIONS

#define FLOAT(name, location) float name; 
#define VEC2(name, location) vec2 name; 
#define VEC3(name, location) vec3 name; 
#define VEC4(name, location) vec4 name; 
#define INT(name, location) int32_t name; 
#define IVEC2(name, location) ivec2 name;
#define IVEC3(name, location) ivec3 name; 
#define IVEC4(name, location) ivec4 name;
#define UINT(name, location) uint32_t name; 
#define UVEC2(name, location) uvec2 name;
#define UVEC3(name, location) uvec3 name; 
#define UVEC4(name, location) uvec4 name; 

#undef DEFINITIONS

#elif defined(EMPTY)

#define FLOAT(name, location) 
#define VEC2(name, location) 
#define VEC3(name, location) 
#define VEC4(name, location) 
#define INT(name, location) 
#define IVEC2(name, location)
#define IVEC3(name, location) 
#define IVEC4(name, location)
#define UINT(name, location) 
#define UVEC2(name, location)
#define UVEC3(name, location) 
#define UVEC4(name, location)

#undef EMPTY

#elif defined(COUNT)

#define FLOAT(name, location) __counter++; 
#define VEC2(name, location) __counter++; 
#define VEC3(name, location) __counter++; 
#define VEC4(name, location) __counter++; 
#define INT(name, location) __counter++; 
#define IVEC2(name, location) __counter++;
#define IVEC3(name, location) __counter++; 
#define IVEC4(name, location) __counter++;
#define UINT(name, location) __counter++; 
#define UVEC2(name, location) __counter++;
#define UVEC3(name, location) __counter++; 
#define UVEC4(name, location) __counter++;

#undef COUNT

#elif defined(FORMAT)
#define FLOAT(name, location) __attributes[__counter++].format = VK_FORMAT_R32_SFLOAT; 
#define VEC2(name, location) __attributes[__counter++].format = VK_FORMAT_R32G32_SFLOAT; 
#define VEC3(name, location) __attributes[__counter++].format = VK_FORMAT_R32G32B32_SFLOAT; 
#define VEC4(name, location) __attributes[__counter++].format = VK_FORMAT_R32G32B32A32_SFLOAT; 
#define INT(name, location) __attributes[__counter++].format = VK_FORMAT_R32_SINT; 
#define IVEC2(name, location) __attributes[__counter++].format = VK_FORMAT_R32G32_SINT;
#define IVEC3(name, location) __attributes[__counter++].format = VK_FORMAT_R32G32B32_SINT; 
#define IVEC4(name, location) __attributes[__counter++].format = VK_FORMAT_R32G32B32A32_SINT;
#define UINT(name, location) __attributes[__counter++].format = VK_FORMAT_R32_UINT; 
#define UVEC2(name, location) __attributes[__counter++].format = VK_FORMAT_R32G32_UINT;
#define UVEC3(name, location) __attributes[__counter++].format = VK_FORMAT_R32G32B32_UINT; 
#define UVEC4(name, location) __attributes[__counter++].format = VK_FORMAT_R32G32B32A32_UINT;

#undef FORMAT

#elif defined(LOCATION)

#define FLOAT(name, l) __attributes[__counter++].location = l; 
#define VEC2(name, l) __attributes[__counter++].location = l; 
#define VEC3(name, l) __attributes[__counter++].location = l; 
#define VEC4(name, l) __attributes[__counter++].location = l; 
#define INT(name, l) __attributes[__counter++].location = l; 
#define IVEC2(name, l) __attributes[__counter++].location = l;
#define IVEC3(name, l) __attributes[__counter++].location = l; 
#define IVEC4(name, l) __attributes[__counter++].location = l;
#define UINT(name, l) __attributes[__counter++].location = l; 
#define UVEC2(name, l) __attributes[__counter++].location = l;
#define UVEC3(name, l) __attributes[__counter++].location = l; 
#define UVEC4(name, l) __attributes[__counter++].location = l;

#undef LOCATION

#elif defined(OFFSET)

#define FLOAT(n, l) __attributes[__counter++].offset = offsetof(BLOCK_NAME, n); 
#define VEC2(n, l) __attributes[__counter++].offset = offsetof(BLOCK_NAME, n); 
#define VEC3(n, l) __attributes[__counter++].offset = offsetof(BLOCK_NAME, n); 
#define VEC4(n, l) __attributes[__counter++].offset = offsetof(BLOCK_NAME, n); 
#define INT(n, l) __attributes[__counter++].offset = offsetof(BLOCK_NAME, n); 
#define IVEC2(n, l) __attributes[__counter++].offset = offsetof(BLOCK_NAME, n);
#define IVEC3(n, l) __attributes[__counter++].offset = offsetof(BLOCK_NAME, n); 
#define IVEC4(n, l) __attributes[__counter++].offset = offsetof(BLOCK_NAME, n);
#define UINT(n, l) __attributes[__counter++].offset = offsetof(BLOCK_NAME, n); 
#define UVEC2(n, l) __attributes[__counter++].offset = offsetof(BLOCK_NAME, n);
#define UVEC3(n, l) __attributes[__counter++].offset = offsetof(BLOCK_NAME, n); 
#define UVEC4(n, l) __attributes[__counter++].offset = offsetof(BLOCK_NAME, n);

#undef OFFSET

#endif