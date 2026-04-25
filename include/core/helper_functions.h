#pragma once
#include <common.h>

struct RenderContext;
struct Allocator;
struct Descriptors;
struct ShaderLoader;
struct Resources;

namespace Helpers {
    API VkDevice device(RenderContext*);

    API Allocator& allocator(RenderContext*);

    API Descriptors& getDescriptors(RenderContext*);

    API ShaderLoader& shaderLoader(RenderContext*);

    API Resources& resources(RenderContext*);
}

