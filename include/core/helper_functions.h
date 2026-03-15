#pragma once
#include <common.h>

struct RenderContext;
struct Allocator;
struct Descriptors;
struct ShaderLoader;

namespace Helpers {
    VkDevice API device(RenderContext*);

    Allocator& API allocator(RenderContext*);

    Descriptors& API getDescriptors(RenderContext*);

    ShaderLoader& API shaderLoader(RenderContext*);
}

