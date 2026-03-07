#pragma once
#include <common.h>
#include <allocator_feature.h>

struct RenderContext;

namespace Helpers {
    VkDevice API device(RenderContext*);

    Allocator& API allocator(RenderContext*);


}

