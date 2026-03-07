#include <helper_functions.h>
#include <render_context.h>

Allocator& Helpers::allocator(RenderContext* ctx) {
    return ctx->Get<Allocator>();
}

VkDevice Helpers::device(RenderContext* context) {
    return context->device();
}