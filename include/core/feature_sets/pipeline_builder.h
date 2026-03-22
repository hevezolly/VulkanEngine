#pragma once

#include <common.h>
#include <vector>
#include <shader_module.h>
#include <shader_loader.h>
#include <descriptor_pool.h>
#include <allocator_feature.h>
#include <helper_functions.h>

struct RenderContext;

struct Pipeline {
    VkPipeline pipeline;
    VkPipelineLayout layout;

    Pipeline(VkDevice d, VkPipeline p, VkPipelineLayout l) : 
        device(d), pipeline(p), layout(l) {}

    virtual RULE_5(Pipeline)

protected:
    VkDevice device;
};

template<typename Self>
struct PipelineBuilder {

    PipelineBuilder(RenderContext& c): context(&c){}
    virtual ~PipelineBuilder(){}

    PipelineBuilder(const PipelineBuilder&) = delete;
    PipelineBuilder& operator=(const PipelineBuilder&) = delete;

    virtual Self& AddShaderStage(const std::string& path, Stage stage) {
        return AddShaderStage(Helpers::shaderLoader(context).Get(path, stage));
    }
    
    virtual Self& AddShaderStage(const ShaderBinary& binary) {
        VkShaderModule vkModule;
        VkShaderModuleCreateInfo moduleInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        moduleInfo.codeSize = binary.size_in_bytes();
        moduleInfo.pCode = binary.spirVWords.data();

        VK(vkCreateShaderModule(Helpers::device(context), &moduleInfo, nullptr, &vkModule))

        shaderModules.emplace_back(vkModule, Helpers::device(context));
        
        VkPipelineShaderStageCreateInfo stageInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        stageInfo.stage = ToVkShaderStage(binary.stage);
        stageInfo.module = vkModule;
        stageInfo.pName = "main";

        stages.push_back(stageInfo);

        return getSelf();
    }

    template<typename T>
    Self& AddLayout() {
        descriptorLayouts.push_back(Helpers::getDescriptors(context).GetLayout<T>());
        return getSelf();
    }

protected:

    virtual Self& getSelf() = 0;

    VkPipelineLayout createLayout() {
        
        VkPipelineLayout layout;

        VkPipelineLayoutCreateInfo pipelineLayout = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        pipelineLayout.setLayoutCount = descriptorLayouts.size();
        pipelineLayout.pSetLayouts = descriptorLayouts.data();
        pipelineLayout.pushConstantRangeCount = 0;
        pipelineLayout.pPushConstantRanges = nullptr;
        pipelineLayout.flags = 0;

        VK(vkCreatePipelineLayout(
            Helpers::device(context), 
            &pipelineLayout, 
            nullptr, 
            &layout
        ));
        return layout;
    }

    RenderContext* context;

    std::vector<ShaderModule> shaderModules;
    std::vector<VkPipelineShaderStageCreateInfo> stages;
    std::vector<VkDescriptorSetLayout> descriptorLayouts;
};