#pragma once
#include <common.h>

struct API InstantMessage{};

struct API EarlyInitMsg{};

struct API InitMsg{};

struct API DestroyMsg{};

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

template<typename T>
struct CanHandle {
    virtual void OnMessage(T*) = 0;
};