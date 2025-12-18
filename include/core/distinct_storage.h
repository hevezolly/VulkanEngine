#pragma once
#include <feature_set.h>
#include <render_context.h>
#include <functional>
#include <unordered_map>
#include <poolable_item.h>

template <class T>
inline void hash_combine(std::size_t& seed, const T& v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

struct TupleHash {
    template <class T>
    std::size_t operator()(const T& t) const {
        std::size_t seed = 0;
        std::apply([&](const auto&... args) {
            ((hash_combine(seed, args)), ...);
        }, t);
        return seed;
    }
};

template <typename ObjetT, typename... Args>
struct DistinctStorage {

    DistinctStorage(RenderContext& context);
    Ref<ObjetT> Get(Args...);
    void Clear();
    inline uint32_t size() {
        return _items.size();
    }

private:
    RenderContext& context;
    std::unordered_map<std::tuple<Args...>, Ref<ObjetT>, TupleHash> _items;
};

template<typename T, typename... Args>
Ref<T> DistinctStorage<T, Args...>::Get(Args... args) {

    std::tuple<Args...> key{std::forward<Args>(args)...};

    auto it = _items.find(key);

    if (it != _items.end()) {
        return it->second;
    }
    else {
        Ref<T> value = context.New<T>(std::forward<Args>(args)...);
        _items.insert({key, value});
        return value;
    }
}

template<typename T, typename... Args>
void DistinctStorage<T, Args...>::Clear() {
    for (auto& pair: _items) {
        context.Delete(pair.second);
    }
    _items.clear();
}

template<typename T, typename... Args>
DistinctStorage<T, Args...>::DistinctStorage(RenderContext& ctxt):
context(ctxt),
_items() {}