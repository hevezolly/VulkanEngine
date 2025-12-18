#pragma once
#include <atomic>
#include <vector>
#include <common.h>
#include <typeindex>
#include <typeinfo>

template <typename T>
std::type_index getTypeId() {
    return std::type_index(typeid(T));
}

struct RenderContext;

struct API FeatureSet {

    FeatureSet(RenderContext&);

    virtual void Destroy();
    virtual void GetRequiredExtentions(std::vector<const char*>& buffer);
    virtual void GetRequiredLayers(std::vector<const char*>& buffer);
        
protected:
    RenderContext& context;
};