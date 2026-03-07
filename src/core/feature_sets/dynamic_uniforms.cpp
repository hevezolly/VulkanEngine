#include <dynamic_uniforms.h>
#include <render_context.h>
#include <resources.h>
#include <frame_dispatcher.h>
#include <device.h>

inline uint64_t align(uint64_t size, VkDeviceSize alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

void ensureCapacity(uint64_t& capacityPerFrame, VkDeviceSize alignment, VkDeviceSize maxSize, uint32_t framesInFlight) {
    capacityPerFrame = std::min(align(capacityPerFrame, alignment), 
        align(maxSize / framesInFlight - alignment, alignment));
}


void DynamicUniforms::OnMessage(InitMsg*) {
    Resources& r = context.Get<Resources>();
    Device& device = context.Get<Device>();

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(device.vkPhysicalDevice, &props);

    minAlignment = props.limits.minUniformBufferOffsetAlignment;
    maxSize = props.limits.maxUniformBufferRange;

    FrameDispatcher& d = context.Get<FrameDispatcher>();
    framesInFlight = d.getFramesInFlight();
    
    ensureCapacity(capacityPerFrame, minAlignment, maxSize, framesInFlight);
    currentBuffer = r.CreateRawBuffer(BufferPreset::UNIFORM, framesInFlight * capacityPerFrame);
    r.GiveName(currentBuffer, "global_dynamic_uniform_buffer");
}

void DynamicUniforms::Reallocate() {
    retirementQueue->push_back(currentBuffer);

    Resources& r = context.Get<Resources>();
    currentBuffer = r.CreateRawBuffer(BufferPreset::UNIFORM, framesInFlight * capacityPerFrame);
    r.GiveName(currentBuffer, "global_dynamic_uniform_buffer");

    allocations->buffer = currentBuffer;
    allocations->offset = allocations.getCurrentFrame() * capacityPerFrame;
    allocations->size = 0;
}

BufferRegion DynamicUniforms::AllocateRange(uint32_t size) {
    uint64_t alignedSize = align(size, minAlignment);

    uint64_t requiredCapacity = capacityPerFrame;
    uint64_t availableSize = requiredCapacity - allocations->size;

    while (availableSize < alignedSize)
    {
        requiredCapacity <<= 1; 
        availableSize = requiredCapacity - allocations->size;
    }

    if (requiredCapacity != capacityPerFrame) {
        capacityPerFrame = requiredCapacity;
        Reallocate();
    }

    uint64_t offset = allocations->offset + allocations->size;
    allocations->size += alignedSize;
    return BufferRegion(currentBuffer, offset, alignedSize);
}

void DynamicUniforms::OnMessage(BeginFrameMsg* m) {

    Resources& r = context.Get<Resources>();

    retirementQueue.SetFrame(m->inFlightFrame);
    for (int i = 0; i < retirementQueue->size(); i++) {
        r.DestroyImmediate(retirementQueue.val()[i]);
    }
    retirementQueue->clear();
    allocations.SetFrame(m->inFlightFrame);

    if (m->inFlightFrame >= framesInFlight) {
        framesInFlight = m->inFlightFrame + 1;
        ensureCapacity(capacityPerFrame, minAlignment, maxSize, framesInFlight);
        Reallocate();
    }

    allocations->buffer = currentBuffer;
    allocations->offset = allocations.getCurrentFrame() * capacityPerFrame;
    allocations->size = 0;
}