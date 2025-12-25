#ifdef UNIFORM_BUFFER
#undef UNIFORM_BUFFER
#endif

#ifdef WRAPPER

#define UNIFORM_BUFFER(name, binding, stage) WRAPPER(Buffer*, name, binding, stage, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, BUFFER_INFO)

#endif