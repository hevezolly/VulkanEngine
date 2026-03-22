#pragma once
#include <common.h>

struct RenderContext;
struct Allocator;
struct Descriptors;
struct ShaderLoader;

namespace Helpers {
    API VkDevice device(RenderContext*);

    API Allocator& allocator(RenderContext*);

    API Descriptors& getDescriptors(RenderContext*);

    API ShaderLoader& shaderLoader(RenderContext*);
}

