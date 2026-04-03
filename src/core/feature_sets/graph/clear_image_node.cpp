#include <clear_image_node.h>

void ClearImageNode::getOutputDependencies(NodeDependency* buffer) {
    *buffer = NodeDependency {
        image.id,
        ResourceState {
            VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        }
    };
}

void ClearImageNode::Record(ExecutionContext commandBuffer) {
    
    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    if (isDepthImage())
        aspect = dsAspects;

    VkImageSubresourceRange range {
        .aspectMask     = aspect,  // or DEPTH_BIT / STENCIL_BIT
        .baseMipLevel   = 0,
        .levelCount     = VK_REMAINING_MIP_LEVELS,    // ← covers all mips
        .baseArrayLayer = 0,
        .layerCount     = VK_REMAINING_ARRAY_LAYERS,  // ← covers all layers
    };

    if (!isDepthImage) {
        vkCmdClearColorImage(
            commandBuffer.commandBuffer->buffer, 
            image->vkImage, 
            VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
            &clearValue.color,
            1,
            &range
        );
    } else {
        vkCmdClearDepthStencilImage(
            commandBuffer.commandBuffer->buffer, 
            image->vkImage,
            VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            &clearValue.depthStencil,
            1,
            &range
        );
    }
}