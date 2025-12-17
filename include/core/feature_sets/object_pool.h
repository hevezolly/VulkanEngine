#pragma once
#include <feature_set.h>
#include <render_context.h>
#include <functional>
#include <vector>
#include <poolable_item.h>

template <typename T>
struct ObjectPool: FeatureSet {

    ObjectPool(RenderContext& context, T* (*factory)(RenderContext&));
    Ref<T> Pool();
    void Return(Ref<T> item);

private:
    T* (*_factory)(RenderContext&);
    std::vector<Ref<T>> _items;
};


template<typename T>
void ObjectPool<T>::Return(Ref<T> item) {
    if (std::is_base_of<PoolableItem, T>::value) {
        context.Get(item).OnReturn();
    }
    _items.push_back(item);
}

template<typename T>
Ref<T> ObjectPool<T>::Pool() {

    Ref<T> item;
    if (_items.size() == 0) {
        item = context.Register(_factory(context));
    }
    else {
        item = _items.pop_back();
    }

    if (std::is_base_of<PoolableItem, T>::value) {
        context.Get(item).OnPool();
    }

    return item;
}

template<typename T>
ObjectPool<T>::ObjectPool(RenderContext& context, T* (*factory)(RenderContext&)): 
FeatureSet(context),
_factory(factory),
_items() 
{}
