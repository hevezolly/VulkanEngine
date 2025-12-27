#include <resources.h>
#include <render_context.h>
#include <device.h>
#include <command_pool.h>
#include <registry.h>


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
    return Memory(mem, context.device(), requirements.size);
}

Buffer Resources::CreateRawBuffer(BufferPreset preset, uint32_t size_bytes) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size_bytes;
    bufferInfo.usage = preset.usageFlags;
    uint32_t usedQueuesCount;
    MemChunk<uint32_t> usedQueues = context.Get<Device>().FillQueueUsages(preset.usedQueues, usedQueuesCount);
    bufferInfo.sharingMode = usedQueuesCount > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.queueFamilyIndexCount = usedQueuesCount;
    bufferInfo.pQueueFamilyIndices = usedQueues.data;
    VkBuffer buffer;

    VK(vkCreateBuffer(context.device(), &bufferInfo, nullptr, &buffer));
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(context.device(), buffer, &memRequirements);
    assert(memRequirements.size == size_bytes);
    Buffer result = Buffer(context.device(), buffer, AllocateMemory(memRequirements, preset.memoryProperties));
    context.Get<Allocator>().Free(usedQueues);
    return result;
}   

Buffer createAndFillBuffer(Resources& r, BufferPreset preset, uint32_t size_bytes, void* data) {
    Buffer buffer = r.CreateRawBuffer(preset, size_bytes);
    {
        auto token = buffer.memory.Map();
        memcpy(token.data(), data, static_cast<size_t>(size_bytes));
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

Image Resources::CreateRawImage(ImageDescription& description, ImageUsage usage) {
    VkImage image;
    VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageInfo.imageType = description.depth == 1 ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_3D;
    imageInfo.extent.width = description.width;
    imageInfo.extent.height = description.height;
    imageInfo.extent.depth = description.depth;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = static_cast<VkImageUsageFlags>(usage);
    uint32_t usedQueuesCount;
    MemChunk<uint32_t> usedQueues = context.Get<Device>().FillQueueUsages(
        QueueType::Transfer | QueueType::Graphics | QueueType::Compute, usedQueuesCount);
    imageInfo.sharingMode = usedQueuesCount > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.queueFamilyIndexCount = usedQueuesCount;
    imageInfo.pQueueFamilyIndices = usedQueues.data;
    
    VkImage image;
    VK(vkCreateImage(context.device(), &imageInfo, nullptr, &image));
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(context.device(), image, &memRequirements);
    
    Image result = Image(image, context.device(), 
        AllocateMemory(memRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), 
        description);

    context.Get<Allocator>().Free(usedQueues);
    return result;
}

int getStbiForceComponents(VkFormat format) 
{
    switch (format) {
        case VK_FORMAT_UNDEFINED:
            return 0;
        case VK_FORMAT_R4G4_UNORM_PACK8:
            return 1;
        case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
            return 2;
        case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
            return 2; 
        case VK_FORMAT_R8_UNORM:
            return 1; 
        case VK_FORMAT_R8_SNORM:
            return 1; 
        case VK_FORMAT_R8_USCALED:
            return 1; 
        case VK_FORMAT_R8_SSCALED:
            return 1; 
        case VK_FORMAT_R8_UINT:
            return 1; 
        case VK_FORMAT_R8_SINT:
            return 1; 
        case VK_FORMAT_R8_SRGB:
            return 1; 
        case VK_FORMAT_R8G8_UNORM:
            return 2; 
        case VK_FORMAT_R8G8_SNORM:
            return 2; 
        case VK_FORMAT_R8G8_USCALED:
            return 2; 
        case VK_FORMAT_R8G8_SSCALED:
            return 2; 
        case VK_FORMAT_R8G8_UINT:
            return ; 
        case VK_FORMAT_R8G8_SINT:
            return 2; 
        case VK_FORMAT_R8G8_SRGB:
            return 2; 
        case VK_FORMAT_R8G8B8_UNORM:
            return 3; 
        case VK_FORMAT_R8G8B8_SNORM:
            return 3; 
        case VK_FORMAT_R8G8B8_USCALED:
            return 3; 
        case VK_FORMAT_R8G8B8_SSCALED:
            return 3; 
        case VK_FORMAT_R8G8B8_UINT:
            return 3; 
        case VK_FORMAT_R8G8B8_SINT:
            return 3; 
        case VK_FORMAT_R8G8B8_SRGB:
            return 3; 
        case VK_FORMAT_B8G8R8_UNORM:
            return 3; 
        case VK_FORMAT_B8G8R8_SNORM:
            return 3; 
        case VK_FORMAT_B8G8R8_USCALED:
            return 3; 
        case VK_FORMAT_B8G8R8_SSCALED:
            return 3; 
        case VK_FORMAT_B8G8R8_UINT:
            return 3; 
        case VK_FORMAT_B8G8R8_SINT:
            return 3; 
        case VK_FORMAT_B8G8R8_SRGB:
            return 3; 
        case VK_FORMAT_R8G8B8A8_UNORM:
            return 4; 
        case VK_FORMAT_R8G8B8A8_SNORM:
            return 4; 
        case VK_FORMAT_R8G8B8A8_USCALED:
            return 4; 
        case VK_FORMAT_R8G8B8A8_SSCALED:
            return 4; 
        case VK_FORMAT_R8G8B8A8_UINT:
            return 4; 
        case VK_FORMAT_R8G8B8A8_SINT:
            return 4; 
        case VK_FORMAT_R8G8B8A8_SRGB:
            return 4; 
        case VK_FORMAT_B8G8R8A8_UNORM:
            return 4; 
        case VK_FORMAT_B8G8R8A8_SNORM:
            return 4; 
        case VK_FORMAT_B8G8R8A8_USCALED:
            return 4; 
        case VK_FORMAT_B8G8R8A8_SSCALED:
            return 4; 
        case VK_FORMAT_B8G8R8A8_UINT:
            return 4; 
        case VK_FORMAT_B8G8R8A8_SINT:
            return 4; 
        case VK_FORMAT_B8G8R8A8_SRGB:
            return 4; 
        default:
            throw std::runtime_error("unsupported image load format");
    }
}

VkFormat getFormatFromNativeComponents(int components) 
{
    switch (components) 
    {
        case 1:
            return VK_FORMAT_R8_UNORM;
        case 2:
            return VK_FORMAT_R8G8_UNORM;
        case 3:
            return VK_FORMAT_R8G8B8_UNORM;
        case 4:
            return VK_FORMAT_R8G8B8A8_UNORM;
        default:
            assert(false);
    }
}

Image Resources::LoadRawImage(ImageUsage usage, const char* path, VkFormat format) {

    int forceComponents = getStbiForceComponents(format);

    RawImageData imageData = context.Get<Registry>().LoadImage(path, forceComponents);
    
    if (forceComponents == 0)
        format = getFormatFromNativeComponents(imageData.num_components);

    ImageDescription description;
    description.width = imageData.x;
    description.height = imageData.y;
    description.depth = 1;
    description.format = format;
    
    Image result = CreateRawImage(description, usage);
    Buffer stagingBuffer = createAndFillBuffer(*this, BufferPreset::STAGING, imageData.size(), imageData.data);

    imageData.Free();

    TransferCommandBuffer cmd = context.Get<CommandPool>().CreateTransferBuffer(true);
    cmd.Begin();
    
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 0;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {
        description.width,
        description.height,
        1
    };

    vkCmdCopyBufferToImage(cmd.buffer, 
        stagingBuffer.vkBuffer, result.vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    cmd.End();
    context.Get<CommandPool>().Submit(cmd);
    vkQueueWaitIdle(context.Get<Device>().queues.get(QueueType::Transfer));


}