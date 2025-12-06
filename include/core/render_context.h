#pragma once

#define GLFW_INCLUDE_NONE
#include <volk.h>
#include <GLFW/glfw3.h>
#include <common.h>
#include <iostream>
#include <unordered_map>
#include <format>
#include <type_traits>
#include <feature_set.h>
#include <handles.h>
#include <typeindex>
#include <typeinfo>

struct API RenderContext {
    VkInstance vkInstance;

    RenderContext();

    VkDevice device();

    void Initialize();

    RULE_5(RenderContext)

    template<typename T, typename... CallArgs>
    RenderContext& WithFeature(CallArgs&&... args) {
        static_assert(std::is_base_of<FeatureSet, T>::value, "T must be derived from FeatureSet");
        
        T* feature = new T(*this, std::forward<CallArgs>(args)...);

        std::type_index typeId = getFeatureId<T>();
        std::cout << "registered " << typeid(T).name() << " id " << typeId.name() << std::endl;

        _features[typeId] = static_cast<FeatureSet*>(feature);
        _featureInitOrder.push_back(typeId);
        return *this;
    }

    template<typename T>
    T* TryGet() {
        static_assert(std::is_base_of<FeatureSet, T>::value, "T must be derived from FeatureSet");
        std::type_index typeId = getFeatureId<T>();
        std::cout << "TryGet " << typeid(T).name() << " id " << typeId.name() << std::endl;
        
        auto it = _features.find(getFeatureId<T>());
        if (it == _features.end())
            return nullptr;
        
        return static_cast<T*>(it->second);
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

        _items.insert({id, static_cast<void*>(item)});
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

    std::unordered_map<std::type_index, FeatureSet*> _features;
    std::vector<std::type_index> _featureInitOrder;
};