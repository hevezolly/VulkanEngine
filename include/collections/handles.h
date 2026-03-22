#pragma once
#include <common.h>
#include <iostream>
#include <optional>

template<typename T>
struct Storage
{
    std::optional<T> item;

    template<typename... Args>
    Storage(Args&&... args) {
        item.emplace(std::forward<Args>(args)...);
    }

    void Delete() {
        item.reset();
    }
};

template <typename T>
struct Ref
{
    T& val() {
        if (!_ptr || !_ptr->item.has_value())
            throw std::runtime_error("dereferencing null ref");
        
        return _ptr->item.value();
    }

    T* try_get() {
        if (!_ptr || !_ptr->item.has_value()) {
            return nullptr;
        }
        return &(_ptr->item->value());
    }


    T& operator*() {
        return val();
    }

    T* operator ->() {
        return &val();
    }

    T* operator &() {
        return &val();
    }

    bool operator==(const Ref<T>& other) const {
        return _ptr == other._ptr;
    }

    bool operator!=(const Ref<T>& other) const {
        return !(*this == other);
    }

    bool isNull() {
        return _ptr == nullptr;
    }

    inline static const Ref Null() {return{};}

    friend struct RenderContext;

private:
    Storage<T>* _ptr;
};

template<typename T>
using Refs = std::vector<Ref<T>>;