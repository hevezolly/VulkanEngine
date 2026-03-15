#include <pipeline_builder.h>

Pipeline::~Pipeline() {
    if (device != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, layout, nullptr);
        vkDestroyPipeline(device, pipeline, nullptr);
    }
    device = VK_NULL_HANDLE;
}

Pipeline& Pipeline::operator=(Pipeline&& other) noexcept {
    if (this == &other)
        return *this;

    pipeline = other.pipeline;
    layout = other.layout;
    device = other.device;

    other.pipeline = VK_NULL_HANDLE;
    other.layout = VK_NULL_HANDLE;
    other.device = VK_NULL_HANDLE;

    return *this;
}

Pipeline::Pipeline(Pipeline&& other) noexcept {
    *this = std::move(other);
}