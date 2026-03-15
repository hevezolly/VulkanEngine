#include <helper_functions.h>
#include <render_context.h>
#include <allocator_feature.h>
#include <descriptor_pool.h>
#include <shader_loader.h>

Allocator& Helpers::allocator(RenderContext* ctx) {
    return ctx->Get<Allocator>();
}

VkDevice Helpers::device(RenderContext* context) {
    return context->device();
}

Descriptors& Helpers::getDescriptors(RenderContext* context) {
    return context->Get<Descriptors>();
}

ShaderLoader& Helpers::shaderLoader(RenderContext* context) {
    return context->Get<ShaderLoader>();
}