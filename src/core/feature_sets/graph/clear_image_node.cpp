#include <clear_image_node.h>

void ClearImageNode::getOutputDependencies(NodeDependency* buffer) {
    *buffer = NodeDependency {
        image.image.id,
        ResourceState {
            VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        }
    };
}

void ClearImageNode::Record(ExecutionContext commandBuffer) {
    
    auto range = image.range;
    if (isDepthImage)
        range.aspectMask = dsAspects;

    if (!isDepthImage) {
        vkCmdClearColorImage(
            commandBuffer.commandBuffer->buffer, 
            image.image->vkImage, 
            VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
            &clearValue.color,
            1,
            &range
        );
    } else {
        vkCmdClearDepthStencilImage(
            commandBuffer.commandBuffer->buffer, 
            image.image->vkImage,
            VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            &clearValue.depthStencil,
            1,
            &range
        );
    }
}