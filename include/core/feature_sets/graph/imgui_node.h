#pragma once

#include <common.h>
#include <render_node.h>
#include <imgui_ui.h>

struct API ImguiNode: RenderNode {

    ImguiNode(RenderContext& ctx, ResourceRef<Image> output);

    QueueType getTargetQueue() { return QueueType::Graphics; }

    uint32_t getInputDependenciesCount() {return 1;}
    uint32_t getOutputDependenciesCount() {return 1;}

    void getInputDependencies(NodeDependency* buffer) {
        *buffer = NodeDependency {
            outputImg.id,
            ResourceState {
                VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                VkImageLayout::VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL
            }
        };
    }

    void getOutputDependencies(NodeDependency* buffer) {
        *buffer = NodeDependency {
            outputImg.id,
            ResourceState {
                VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                VkImageLayout::VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL
            }
        };
    }

    void Record(ExecutionContext context);

private:
    ResourceRef<Image> outputImg;
};