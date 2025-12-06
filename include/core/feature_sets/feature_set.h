#pragma once
#include <atomic>
#include <vector>

extern std::atomic_int TypeIdCounter;

template <typename T>
int getFeatureId() {
    static_assert(std::is_base_of<FeatureSet, T>::value, "T must be derived from FeatureSet");    
    static int id = ++TypeIdCounter;
    return id;
}

struct FeatureSet {

protected:
    virtual void PreInit();
    virtual void Init();
    virtual void Destroy();
    virtual void GetRequiredExtentions(std::vector<const char*>& buffer);
    virtual void GetRequiredLayers(std::vector<const char*>& buffer);

    friend struct RenderContext;
    RenderContext* context;    
};