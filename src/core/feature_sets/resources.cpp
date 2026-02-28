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
    _buffers.type = ResourceType::Buffer;
    _images.type = ResourceType::Image;
    _samplers.type = ResourceType::Sampler;
    vkGetPhysicalDeviceMemoryProperties(
        context.Get<Device>().vkPhysicalDevice, &vkMemProperties);
}

Memory Resources::AllocateMemory(
    VkMemoryRequirements requirements, 
    VkMemoryPropertyFlags properties, uint32_t usable_size
) 
{
    VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};

    allocInfo.allocationSize = requirements.size;
    allocInfo.memoryTypeIndex = findMemType(requirements, properties, vkMemProperties);

    VkDeviceMemory mem;
    VK(vkAllocateMemory(context.device(), &allocInfo, nullptr, &mem));
    return Memory(mem, context.device(), usable_size == 0 ? requirements.size : usable_size);
}

Buffer createStandaloneBuffer(RenderContext& c, BufferPreset preset, uint32_t size_bytes) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size_bytes;
    bufferInfo.usage = preset.usageFlags;
    uint32_t usedQueuesCount;
    MemChunk<uint32_t> usedQueues = c.Get<Device>().FillQueueUsages(preset.usedQueues, usedQueuesCount);
    bufferInfo.sharingMode = usedQueuesCount > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.queueFamilyIndexCount = usedQueuesCount;
    bufferInfo.pQueueFamilyIndices = usedQueues.data;
    VkBuffer buffer;

    VK(vkCreateBuffer(c.device(), &bufferInfo, nullptr, &buffer));
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(c.device(), buffer, &memRequirements);
    Buffer result = Buffer(c, buffer, c.Get<Resources>().AllocateMemory(
        memRequirements, preset.memoryProperties, size_bytes));
    c.Get<Allocator>().Free(usedQueues);
    return result;
}

ResourceRef<Buffer> Resources::CreateRawBuffer(BufferPreset preset, uint32_t size_bytes) {
    return Register(createStandaloneBuffer(context, preset, size_bytes), ResourceState{});
}

Buffer createAndFillBuffer(RenderContext& context, BufferPreset preset, uint32_t size_bytes, void* data) {
    Buffer buffer = createStandaloneBuffer(context, preset, size_bytes);
    {
        auto token = buffer.memory.Map();
        memcpy(token.data(), data, static_cast<size_t>(size_bytes));
    }
    return buffer;
}

ResourceRef<Buffer> Resources::CreateRawBuffer(BufferPreset preset, uint32_t size_bytes, void* data) {

    if ((preset.memoryProperties & (BufferPreset::STAGING.memoryProperties)) == 
        (BufferPreset::STAGING.memoryProperties)) {

        ResourceId id = _buffers.Insert(createAndFillBuffer(context, preset, size_bytes, data));
        return {id, &_buffers};
    }

    Buffer stagingBuffer = createAndFillBuffer(context, BufferPreset::STAGING, size_bytes, data);
    ResourceRef<Buffer> outputBuffer = CreateRawBuffer(preset, size_bytes);
    TransferCommandBuffer cmd = context.Get<CommandPool>().CreateTransferBuffer(true);
    cmd.Begin();
    cmd.CopyBufferRegion(stagingBuffer.vkBuffer, outputBuffer->vkBuffer, size_bytes);
    cmd.End();
    context.Get<CommandPool>().Submit(cmd);
    vkQueueWaitIdle(context.Get<Device>().queues.get(QueueType::Transfer));

    return outputBuffer;
}

ResourceRef<Image> Resources::CreateImage(const ImageDescription& description, ImageUsage usage) {
    VkImage image;
    VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageInfo.imageType = description.depth == 1 ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_3D;
    imageInfo.extent.width = description.width;
    imageInfo.extent.height = description.height;
    imageInfo.extent.depth = description.depth;
    imageInfo.mipLevels = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.arrayLayers = 1;
    imageInfo.format = description.format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = static_cast<VkImageUsageFlags>(usage);
    uint32_t usedQueuesCount;
    MemChunk<uint32_t> usedQueues = context.Get<Device>().FillQueueUsages(
        QueueType::Transfer | QueueType::Graphics | QueueType::Compute, usedQueuesCount);
    imageInfo.sharingMode = usedQueuesCount > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.queueFamilyIndexCount = usedQueuesCount;
    imageInfo.pQueueFamilyIndices = usedQueues.data;

    VK(vkCreateImage(context.device(), &imageInfo, nullptr, &image));
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(context.device(), image, &memRequirements);
    
    ResourceRef<Image> result = Register(Image(image, context, 
        AllocateMemory(memRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), 
        description), ResourceState{});

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
            return 2; 
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
            return VK_FORMAT_R8G8B8_SRGB;
        case 4:
            return VK_FORMAT_R8G8B8A8_SRGB;
        default:
            assert(false);
    }
}

ResourceRef<Image> Resources::LoadImage(ImageUsage usage, const char* path, VkFormat format) {

    int forceComponents = getStbiForceComponents(format);

    RawImageData imageData = context.Get<Registry>().LoadImage(path, forceComponents);
    
    if (forceComponents == 0)
        format = getFormatFromNativeComponents(imageData.num_components);

    ImageDescription description;
    description.width = imageData.x;
    description.height = imageData.y;
    description.depth = 1;
    description.format = format;
    
    ResourceRef<Image> result = CreateImage(description, usage | ImageUsage::TransferDst);
    GiveName(result, path);
    Buffer stagingBuffer = createAndFillBuffer(context, BufferPreset::STAGING, imageData.size(), imageData.data);
    imageData.Free();
    TransferCommandBuffer cmd = context.Get<CommandPool>().CreateTransferBuffer(true);
    cmd.Begin();

    cmd.ImageBarrier(result, 
        ResourceState {
            VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        }
    );
    
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {
        description.width,
        description.height,
        1
    };

    vkCmdCopyBufferToImage(cmd.buffer, 
        stagingBuffer.vkBuffer, result->vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);


    cmd.End();
    context.Get<CommandPool>().Submit(cmd);

    vkQueueWaitIdle(context.Get<Device>().queues.get(QueueType::Transfer));

    return result;
}

ResourceRef<Sampler> Resources::CreateSampler(const SamplerFilter& filter, const SamplerAddressMode& addressMode) {
    VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

    samplerInfo.magFilter = filter.magFilter;
    samplerInfo.minFilter = filter.minFilter;
    samplerInfo.addressModeU = addressMode.uMode;
    samplerInfo.addressModeV = addressMode.vMode;
    samplerInfo.addressModeW = addressMode.wMode;

    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    VkSampler sampler;
    VK(vkCreateSampler(context.device(), &samplerInfo, nullptr, &sampler));

    return {_samplers.Insert(Sampler(sampler, context)), &_samplers};
}

void Resources::OnMessage(DestroyMsg*) {
    _buffers.clear();
    _samplers.clear();
    _images.clear();
    _states.clear();
}

ResourceState Resources::GetState(ResourceId id) {
    auto it = _states.find(id);
    if (it != _states.end())
        return it->second;
    
    return ResourceState();
}

void Resources::SetState(ResourceId id, const ResourceState& state) {
    _states[id] = state;
}

ResourceState Resources::UpdateState(ResourceId id, const ResourceState& state) {
    ResourceState old = GetState(id);
    SetState(id, state);
    return old;
}

const std::string& Resources::GetName(ResourceId id) {
    auto it = _names.find(id);
    if (it != _names.end())
        return it->second;
    
    return "";
}


void Resources::GiveName(ResourceId id, const std::string& name) {
    _names[id] = name;
#ifdef ENABLE_VULKAN_VALIDATION
    switch (id.type())
    {
        case ResourceType::Buffer:
            context.NameVkObject(
                VkObjectType::VK_OBJECT_TYPE_BUFFER, 
                (uint64_t)Get<Buffer>(id)->vkBuffer,
                name);
            break;
        case ResourceType::Image: {
            auto& image = *Get<Image>(id);
            context.NameVkObject(
                VkObjectType::VK_OBJECT_TYPE_IMAGE, 
                (uint64_t)image.vkImage,
                name);
            context.NameVkObject(
                VkObjectType::VK_OBJECT_TYPE_IMAGE_VIEW, 
                (uint64_t)image.view->vkImageView,
                name);
            break;
        }
        case ResourceType::Sampler:
            context.NameVkObject(
                VkObjectType::VK_OBJECT_TYPE_SAMPLER, 
                (uint64_t)Get<Sampler>(id)->vkSampler,
                name);
            break;
        default:
            break;
    }
#endif
}

void Resources::DestroyImmediate(ResourceId resource) {

    switch (resource.type())
    {
    case ResourceType::Buffer:
        _buffers.TryRemove(resource);
        break;
    case ResourceType::Image:
        _images.TryRemove(resource);
        break;
    case ResourceType::Sampler:
        _samplers.TryRemove(resource);
    default:
        break;
    }
    _states.erase(resource);
    _synchronization.erase(resource);
}

bool Resources::ResourceRequiresSynchronization(ResourceId resource) {
    return _synchronization.find(resource) != _synchronization.end();
}

Ref<Semaphore> Resources::ExtractSyncContext(ResourceId resource) {
    auto it = _synchronization.find(resource);

    if (it == _synchronization.end())
        return Ref<Semaphore>::Null();
    auto result = it->second;
    _synchronization.erase(it);
    return result; 
}

void Resources::SetSynchronizationContext(ResourceId resource, Ref<Semaphore> sync) {
    _synchronization[resource] = sync;
}
