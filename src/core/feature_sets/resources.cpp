#include <resources.h>
#include <render_context.h>
#include <device.h>
#include <command_pool.h>


uint32_t findMemType(
    VkMemoryRequirements requirements, 
    VkMemoryPropertyFlags properties,
    VkPhysicalDeviceMemoryProperties deviceMemProperties
) {
    for (uint32_t i = 0; i < deviceMemProperties.memoryTypeCount; i++) {
        if ((requirements.memoryTypeBits & (1 << i)) && 
            (deviceMemProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
        }
    }

    throw std::runtime_error("sutable memory type is not found!");
}

void Resources::OnMessage(InitMsg*) {
    vkGetPhysicalDeviceMemoryProperties(
        context.Get<Device>().vkPhysicalDevice, &vkMemProperties);
}

Memory Resources::AllocateMemory(VkMemoryRequirements requirements, VkMemoryPropertyFlags properties) 
{
    VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};

    allocInfo.allocationSize = requirements.size;
    allocInfo.memoryTypeIndex = findMemType(requirements, properties, vkMemProperties);

    VkDeviceMemory mem;
    VK(vkAllocateMemory(context.device(), &allocInfo, nullptr, &mem));

    return Memory(mem, context.device());
}

Buffer Resources::CreateRawBuffer(BufferPreset preset, uint32_t size_bytes) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size_bytes;
    bufferInfo.usage = preset.usageFlags;

    std::vector<uint32_t> usedQueues;
    context.Get<Device>().FillQueueUsages(preset.usedQueues, &usedQueues);

    bufferInfo.sharingMode = usedQueues.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.queueFamilyIndexCount = usedQueues.size();
    bufferInfo.pQueueFamilyIndices = usedQueues.data();

    VkBuffer buffer;

    VK(vkCreateBuffer(context.device(), &bufferInfo, nullptr, &buffer));
    
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(context.device(), buffer, &memRequirements);

    return Buffer(context.device(), buffer, size_bytes, AllocateMemory(memRequirements, preset.memoryProperties));
}

Buffer createAndFillBuffer(Resources& r, BufferPreset preset, uint32_t size_bytes, void* data) {
    Buffer buffer = r.CreateRawBuffer(preset, size_bytes);
    {
        auto token = buffer.Map();
        memcpy(token.data, data, (size_t)size_bytes);
    }
    return buffer;
}

Buffer Resources::CreateRawBuffer(BufferPreset preset, uint32_t size_bytes, void* data) {

    if ((preset.memoryProperties & (BufferPreset::STAGING.memoryProperties)) == 
        (BufferPreset::STAGING.memoryProperties))
        return createAndFillBuffer(*this, preset, size_bytes, data);

    Buffer stagingBuffer = createAndFillBuffer(*this, BufferPreset::STAGING, size_bytes, data);
    Buffer outputBuffer = CreateRawBuffer(preset, size_bytes);
    TransferCommandBuffer cmd = context.Get<CommandPool>().CreateTransferBuffer(true);
    
    cmd.Begin();
    cmd.CopyBufferRegion(stagingBuffer.vkBuffer, outputBuffer.vkBuffer, size_bytes);
    cmd.End();
    context.Get<CommandPool>().Submit(cmd);
    vkQueueWaitIdle(context.Get<Device>().queues.get(QueueType::Transfer));

    return outputBuffer;
}