#pragma once

#include <volk.h>
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include <common.h>
#include <iostream>
#include "window.h"
#include "swap_chain.h"
#include <unordered_map>
#include <format>
#include <type_traits>

template <typename T>
struct API Ref
{
    unsigned long id;
};

struct Device;

struct API RenderContext {
    VkInstance vkInstance;
    Window* window;
    Device* device;
    SwapChain* swapChain;

    RenderContext(bool enableDebugLayer);

    void Initialize();

    RULE_5(RenderContext)

    template<typename T>
    RenderContext& WithFeature(T* feature = nullptr) {
        static_assert(std::is_base_of<FeatureSet, T>::value, "T must be derived from FeatureSet");
        
        if (!feature)
            T* feature = new T();

        feature->context = this;
        int typeId = getFeatureId<T>();
        _features.resize(typeId+1, nullptr);
        if (features[typeId] != nullptr)
            throw std::runtime_error("feature was already added");
        _features.insert(, static_cast<FeatureSet*>(feature));
        _featureInitOrder.push_back(getFeatureId<T>());
        return &this;
    }

    template<typename T>
    T* TryGet() {
        static_assert(std::is_base_of<FeatureSet, T>::value, "T must be derived from FeatureSet");

        int typeId = getFeatureId<T>();
        if (_features.size() <= typeId)
            return nullptr;
        auto it = _features[getFeatureId<T>()];
        if (it == nullptr)
            return nullptr;
        
        return static_cast<T*>(it);
    }

    template<typename T>
    T& Get() {
        T* result = TryGet<T>();
        if (result == nullptr)
            throw std::runtime_error("feature not found");
        
        return *result;
    }

    template<typename T>
    bool Has() {
        return TryGet<T>() != nullptr;
    }

    template<typename T>
    Ref<T> Register(T* item) {
        unsigned long id = _counter++;

        _items.insert(id, static_cast<void*>(item));
        _initOrder.push_back(id);

        return Ref<T> {id};
    }

    template<typename T>
    T& Get(Ref<T> handle) {
        auto it = _items.find(handle.id);
        if (it == _items.end())
            throw std::runtime_error("handle not found");
        
        return *static_cast<T*>(it->second);
    }

    template<typename T>
    T* TryGet(Ref<T> handle) {
        auto it = _items.find(handle.id);
        if (it == _items.end())
            return nullptr;
        
        return static_cast<T*>(it->second);
    }

    template<typename T>
    T* Extract(Ref<T> handle) {
        auto it = _items.find(handle.id);
        if (it == _items.end())
            throw std::runtime_error("handle not found");

        T* pointer = static_cast<T*>(it->second);
        _items.erase(it);
        return pointer;
    }
    
private:

    long _counter;
    std::unordered_map<unsigned long, void*> _items;
    std::vector<unsigned long> _initOrder;

    std::vector<FeatureSet*> _features;
    std::vector<int> _featureInitOrder;

#ifdef ENABLE_VULKAN_VALIDATION
    VkDebugUtilsMessengerEXT vkDebugMessenger;
#endif
};