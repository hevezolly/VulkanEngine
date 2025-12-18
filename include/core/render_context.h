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

struct API DestructorPair {
    const void* object_ptr;
    void(*destroyer_func)(const void*); // A raw function pointer
};

template <typename T>
DestructorPair make_dtor_pair(T* obj) {
    return {
        obj,
        [](const void* p) { delete static_cast<const T*>(p); } // Lambda acts as function pointer
    };
}

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

        _features[typeId] = static_cast<FeatureSet*>(feature);
        _featureInitOrder.push_back(typeId);
        return *this;
    }

    template<typename T>
    T* TryGet() {
        static_assert(std::is_base_of<FeatureSet, T>::value, "T must be derived from FeatureSet");
        std::type_index typeId = getFeatureId<T>();
        
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

    template<typename T, typename... Args>
    Ref<T> New(Args&&... args) {

        Storage<T>* item = new Storage<T>(std::forward<Args>(args)...);

        _initOrder.push_back(make_dtor_pair(item));

        Ref<T> r;
        r._ptr = item;
        return r;
    }

    template<typename T, typename... Args>
    Refs<T> NewRefs(uint32_t size, Args&&... args) {

        Refs<T> refs;
        refs.reserve(size);

        for (int i = 0; i < size; i++) 
        {
            Storage<T>* item = new Storage<T>(std::forward<Args>(args)...);
            _initOrder.push_back(make_dtor_pair(item));
            Ref<T> r;
            r._ptr = item;
            refs.push_back(r);
        }

        return refs;
    }

    template<typename T>
    T& Get(Ref<T> handle) {
        return *handle;
    }

    template<typename T>
    T* TryGet(Ref<T> handle) {
        return handle.try_get();
    }

    template<typename T>
    void Delete(Ref<T> handle) {
        handle._ptr->Delete();
    }
    
private:
    std::vector<DestructorPair> _initOrder;

    std::unordered_map<std::type_index, FeatureSet*> _features;
    std::vector<std::type_index> _featureInitOrder;
};