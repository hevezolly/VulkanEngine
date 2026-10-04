#pragma once
#include <common.h>
#include <resource_id.h>
#include <vector>
#include <optional>
#include <unordered_set>
#include <array>
#include <utility>

template<typename T>
struct ResourceStorage {

    ResourceType type;

    ResourceStorage(): 
        type(ResourceType::Other), 
        _availableIds(), 
        _generations(),
        _storage() {}

    virtual RULE_NO(ResourceStorage)

    virtual void clear() {
        _availableIds.clear();
        _generations.clear();
        _storage.clear();
    }

    T& GetUnchecked(ResourceId id) {
        return _storage[id.index()].value();
    }

    const T& GetUnchecked(ResourceId id) const {
        return _storage[id.index()].value();
    }

    T* TryGet(ResourceId id) {
        if (id.type() != type)
            return nullptr;
        uint32_t generation = id.generation();
        uint32_t index = id.index();

        if (index >= _generations.size())
            return nullptr;

        if (_generations[index] != generation)
            return nullptr;

        return &_storage[index].value();
    }

    virtual ResourceId Insert(T&& item) {
        
        ResourceId id;
        if (_availableIds.size() > 0) {
            id = _availableIds.back();
            _availableIds.pop_back();
        } else {
            id = ResourceId::Compose(type, _storage.size());
        }

        uint32_t index = id.index();
        
        if (index >= _storage.size()) {
            _generations.resize(index + 1);
            for (int i = _storage.size(); i <= index; i++) {
                _storage.push_back(std::nullopt);
            }
        }


        _generations[index] = id.generation();
        ASSERT(!_storage[index].has_value());
        _storage[index].emplace(std::forward<T>(item));

        return id;
    }

    virtual bool TryRemove(ResourceId id) {


        if (id.type() != type) {
            return false;
        }

        uint32_t generation = id.generation();
        uint32_t index = id.index();

        if (index >= _generations.size()) {
            return false;
        }

        if (_generations[index] != generation) {
            return false;
        }

        if (!_storage[index].has_value()) {
            return false;
        }

        RemoveUnchecked(id);
        return true;
    }

    void Replace(ResourceId& id, T&& newValue) {
        ASSERT(id.index() < _storage.size());

        uint32_t index = id.index();
        _storage[index].reset();
        _storage[index].emplace(std::forward<T>(newValue));
        id = id.NextGeneration();
        _generations[index] = id.generation();
    }

protected:
    void RemoveUnchecked(ResourceId id) {
        _availableIds.push_back(id.NextGeneration());
        _storage[id.index()].reset();
        _generations[id.index()] = UINT32_MAX;
    }

    std::vector<ResourceId> _availableIds;
    std::vector<uint32_t> _generations;
    std::vector<std::optional<T>> _storage;
};

template<typename T>
ResourceStorage<T>::~ResourceStorage() {
    clear();
}

template<typename T>
struct ResourceRef {
    ResourceId id;

    ResourceRef(): id(ResourceId::Compose(ResourceType::Other, 0, UINT32_MAX)), _storage(nullptr) {}
    ResourceRef(ResourceId i, ResourceStorage<T>* storage): id(i), _storage(storage) {}

    T& val() {
        ASSERT(_storage != nullptr);
        return _storage->GetUnchecked(id);
    }

    const T& val() const {
        ASSERT(_storage != nullptr);
        return _storage->GetUnchecked(id);
    }

    T* try_get() {
        ASSERT(_storage != nullptr);
        return _storage->TryGet(id);
    }

    T& operator*() {
        return val();
    }

    T* operator ->() {
        return &val();
    }

    const T* operator ->() const {
        return &val();
    }

    T* operator &() {
        return &val();
    }

    bool operator==(const ResourceRef<T>& other) const {
        return id == other.id && _storage == other._storage; 
    }

    bool operator!=(const ResourceRef<T>& other) const {
        return !(*this == other);
    }

    operator ResourceId() const {
        return id;
    }

    void Replace(T&& newValue) {
        _storage->Replace(id, std::move(newValue));
    }

private: 
    ResourceStorage<T>* _storage; 
};

template<typename T>
using ResourceRefs = std::vector<ResourceRef<T>>;

