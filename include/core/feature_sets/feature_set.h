#pragma once
#include <atomic>
#include <vector>
#include <common.h>
#include <typeindex>
#include <typeinfo>
#include <messages.h>

template <typename T>
std::type_index getTypeId() {
    return std::type_index(typeid(T));
}

struct RenderContext;

struct API FeatureSet {

    virtual ~FeatureSet(){};
    FeatureSet(RenderContext& c): context(c){};
        
protected:
    RenderContext& context;
};