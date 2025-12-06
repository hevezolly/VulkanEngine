#pragma once
#include <atomic>
#include <vector>
#include <common.h>
#include <typeindex>
#include <typeinfo>

template <typename T>
std::type_index getFeatureId() {
    static_assert(std::is_base_of<FeatureSet, T>::value, "T must be derived from FeatureSet");    
    return std::type_index(typeid(T));
}

struct RenderContext;

struct API FeatureSet {

    FeatureSet(RenderContext&);

    virtual void PreInit();
    virtual void Init();
    virtual void Destroy();
    virtual void GetRequiredExtentions(std::vector<const char*>& buffer);
    virtual void GetRequiredLayers(std::vector<const char*>& buffer);
        
protected:
    RenderContext& context;
};