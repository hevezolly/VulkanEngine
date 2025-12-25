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
#include <messages.h>
#include <queue>

struct API DestructorPair {
    const void* object_ptr;
    void(*destroyer_func)(const void*); // A raw function pointer
};

struct API MessageHandler {
    void* handlerPtr;
    void(*handler_func)(void*, void*);
};

struct API Message {
    void* message;
    std::type_index type;
    bool bottomToTop;
};

template <typename T>
Message make_message(T* message, bool bottomToTop) {
    return {
        message,
        getTypeId<T>(),
        bottomToTop
    };
}

template <typename T>
DestructorPair make_dtor_pair(T* obj) {
    return {
        obj,
        [](const void* p) { delete static_cast<const T*>(p); } // Lambda acts as function pointer
    };
}

template<typename T>
MessageHandler make_handler(CanHandle<T>* handler) {
    return {
        handler,
        [](void* h, void* m) { 
            static_cast<CanHandle<T>*>(h)->
                    OnMessage(static_cast<T*>(m));
        }
    };
}

template<class>
inline constexpr bool dependent_false_v = false;

struct API RenderContext {
    VkInstance vkInstance;
    uint32_t apiVersion;

    RenderContext();

    VkDevice device();

    void Initialize();

    RULE_5(RenderContext)

    template<typename T, typename... CallArgs>
    RenderContext& WithFeature(CallArgs&&... args) {
        static_assert(std::is_base_of<FeatureSet, T>::value, "T must be derived from FeatureSet");
        
        T* feature = new T(*this, std::forward<CallArgs>(args)...);

        std::type_index typeId = getTypeId<T>();

        _features[typeId] = static_cast<FeatureSet*>(feature);

        if (std::find(_featureInitOrder.begin(), _featureInitOrder.end(), typeId) == _featureInitOrder.end())
            _featureInitOrder.push_back(typeId);
        return *this;
    }

    template<typename T>
    T* TryGet() {
        static_assert(std::is_base_of<FeatureSet, T>::value, "T must be derived from FeatureSet");
        std::type_index typeId = getTypeId<T>();
    
        auto it = _features.find(getTypeId<T>());
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

        Storage<T>* item = ConstructStorage<T, Args...>(std::forward<Args>(args)...);

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
            Storage<T>* item = ConstructStorage<T, Args...>(std::forward<Args>(args)...);
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

    template<typename T>
    void Send(T* message=nullptr, bool bottomToTop=false) {
        std::type_index id = getTypeId<T>();
        auto it = _messageHandlers.find(id);

        if (it == _messageHandlers.end()) {
            _messageHandlers.insert({id, std::vector<MessageHandler>()});

            for (int i = 0; i < _featureInitOrder.size(); i++) {
                FeatureSet* feature = _features[_featureInitOrder[i]];
                auto h = dynamic_cast<CanHandle<T>*>(feature);
                if (!h)
                    continue;

                _messageHandlers[id].push_back(make_handler(h));
            }
        }

        if (_messageHandlers[id].empty())
            return;

        if (std::is_base_of<InstantMessage, T>::value) {
            HandleMessageNow(make_message(message, bottomToTop));
        }
        else {
            _messages.push(make_message(message, bottomToTop));
            HandleMessages();
        }
    }

    template<typename T>
    void Send(T&& message, bool bottomToTop=false) {
        T copy = message;
        Send(&copy, bottomToTop);
    }
    
private:

    template<typename T, typename... Args>
    Storage<T>* ConstructStorage(Args&&... args) {
        if constexpr (std::is_constructible_v<T, Args&&...>)
            return new Storage<T>(std::forward<Args>(args)...);
        else if constexpr (std::is_constructible_v<T, RenderContext*, Args&&...>)
            return new Storage<T>(this, std::forward<Args>(args)...);
        else if constexpr (std::is_constructible_v<T, RenderContext&, Args&&...>)
            return new Storage<T>(*this, std::forward<Args>(args)...);
        else
            static_assert(dependent_false_v<T>, "No matching T constructor for these arguments");
    }

    void HandleMessageNow(Message&&);
    void HandleMessages();

    std::vector<DestructorPair> _initOrder;

    std::unordered_map<std::type_index, FeatureSet*> _features;
    std::vector<std::type_index> _featureInitOrder;

    std::unordered_map<std::type_index, std::vector<MessageHandler>> _messageHandlers;
    std::queue<Message> _messages;

    bool _handling;
};