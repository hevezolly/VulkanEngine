#pragma once
#include <common.h>

struct API EarlyInitMessage{};
struct API InitMessage{};

template<typename T>
struct CanHandle {
    virtual void OnMessage(T*) = 0;
};