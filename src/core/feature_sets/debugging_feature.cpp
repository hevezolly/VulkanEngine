#include <debugging_feature.h>
#include <iostream>
#include <render_context.h>


static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    }

    return VK_FALSE;
}

DebuggingFeature::DebuggingFeature(RenderContext& context): FeatureSet(context) {
    messengerCreateInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    messengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    messengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    messengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    messengerCreateInfo.pfnUserCallback = debugCallback;
}

void DebuggingFeature::PreInit() {
    VK(vkCreateDebugUtilsMessengerEXT(context.vkInstance, &messengerCreateInfo, nullptr, &vkDebugMessenger));
}

void DebuggingFeature::Destroy() {
    vkDestroyDebugUtilsMessengerEXT(context.vkInstance, vkDebugMessenger, nullptr);
}

void DebuggingFeature::GetRequiredExtentions(std::vector<const char*>& buffer) {
    buffer.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
}

void DebuggingFeature::GetRequiredLayers(std::vector<const char*>& buffer) {
    buffer.push_back("VK_LAYER_KHRONOS_validation");
}