#include <command_pool.h>
#include <render_context.h>
#include <graphics_feature.h>

VkCommandPool CreateCommandPool(VkDevice device, uint32_t queueFamily) {
    VkCommandPool pool;

    VkCommandPoolCreateInfo info {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    info.queueFamilyIndex = queueFamily;

    VK(vkCreateCommandPool(device, &info, nullptr, &pool));
}

void CommandPools::Init() {

    graphicsCommandPool = VK_NULL_HANDLE;
    
    if (context.Has<GraphicsFeature>()) {
        graphicsCommandPool = CreateCommandPool(
            context.device(), 
            context.Get<Device>().queueFamilies.get(QueueType::Graphics));
    }
}

void CommandPools::Destroy() {
    if (graphicsCommandPool != VK_NULL_HANDLE)
        vkDestroyCommandPool(context.device(), graphicsCommandPool, nullptr);
}

