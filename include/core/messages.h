#pragma once
#include <common.h>

struct API InstantMessage{};

struct API EarlyInitMsg{};

struct API InitMsg{};

struct API DestroyMsg{};
struct API EarlyDestroyMsg{};

struct API BeginFrameMsg{
    const uint32_t inFlightFrame;
};

struct API BeginFrameLateMsg{
    const uint32_t inFlightFrame;
};

struct API EndFrameMsg {};

struct API CollectInstanceRequirementsMsg: InstantMessage{
    std::vector<const char*>* extentions;
    std::vector<const char*>* layers;

    CollectInstanceRequirementsMsg(std::vector<const char*>* e, std::vector<const char*>* l):
        InstantMessage(), extentions(e), layers(l)
    {}
};

struct API CollectDeviceRequirementsMsg: InstantMessage {
    std::vector<const char*>* extentions;
    
    CollectDeviceRequirementsMsg(std::vector<const char*>* e):
        InstantMessage(), extentions(e)
    {}
};

struct API CheckDeviceAppropriateMsg: InstantMessage {
    bool appropriate;
    const VkPhysicalDevice device;

    CheckDeviceAppropriateMsg(const VkPhysicalDevice d):
        appropriate(true),
        device(d){}
};

struct API CollectRequiredQueueTypesMsg: InstantMessage {
    QueueTypes requiredTypes;

    CollectRequiredQueueTypesMsg(): requiredTypes(0){}
};

template<typename T>
struct CanHandle {
    virtual void OnMessage(T*) = 0;
};