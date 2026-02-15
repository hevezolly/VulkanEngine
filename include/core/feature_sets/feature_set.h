#pragma once
#include <common.h>
#include <atomic>
#include <vector>
#include <typeindex>
#include <typeinfo>
#include <messages.h>

struct API TypeId {

    size_t hash_code() const {
        return _info->hash_code();
    }

    TypeId(const std::type_info& i): _info(&i) {}

    TypeId(const TypeId& other) noexcept {
        _info = other._info;
    }

    TypeId& operator=(const TypeId& other) noexcept {
        _info = other._info;

        return *this;
    }

    TypeId(TypeId&& other) noexcept {
        _info = other._info;
    }

    TypeId& operator=(TypeId&& other) noexcept {
        _info = other._info;
    }

    bool operator==(const TypeId& other) const {
        return *_info == *other._info;
    }

    bool operator!=(const TypeId& other) const {
        return !(*this == other);
    }

private:
    const std::type_info* _info;

};

namespace std {
    template <>
    struct hash<TypeId> {
        size_t operator()(const TypeId& _Keyval) const noexcept {
            return _Keyval.hash_code();
        }
    };
}

template <typename T>
TypeId getTypeId() {
    const std::type_info& id = typeid(T);
    return TypeId(id);
}

struct RenderContext;

struct API FeatureSet {

    virtual ~FeatureSet(){};
    FeatureSet(RenderContext& c): context(c){};
        
protected:
    RenderContext& context;
};