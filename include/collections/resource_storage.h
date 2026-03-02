#pragma once
#include <common.h>
#include <resource_id.h>
#include <vector>
#include <optional>
#include <unordered_set>
#include <array>

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
        assert(!_storage[index].has_value());
        _storage[index].emplace(std::forward(item));

        return id;
    }

    virtual bool TryRemove(ResourceId id) {
        if (id.type() != type)
            return false;

        uint32_t generation = id.generation();
        uint32_t index = id.index();

        if (index >= _generations.size())
            return false;

        if (_generations[index] != generation)
            return false;

        if (!_storage[index].has_value())
            return false;

        RemoveUnchecked(id.index());
    }

    void Replace(ResourceId& id, T&& newValue) {
        assert(id.index() < _storage.size());

        uint32_t index = id;
        _storage[index].reset();
        _storage[index].emplace(std::forward(newValue));
        id = id.NextGeneration();
        _generations[index] = id.generation();
    }

protected:
    void RemoveUnchecked(uint32_t index) {
        _availableIds.push_back(id.NextGeneration());
        _storage[index].reset();
        _generations[index] = UINT32_MAX;
    }

    std::vector<ResourceId> _availableIds;
    std::vector<uint32_t> _generations;
    std::vector<std::optional<T>> _storage;
};

template<typename T>
struct ResourceStorageWithChildren: ResourceStorage<T> {

private:
struct ChildData {
    uint32_t child;
    uint32_t parent;

    ChildData(): 
        parent(UINT32_MAX), 
        child(UINT32_MAX)
        {}
};

std::vector<ChildData> _childData;

public:

    ResourceStorageWithChildren(): ResourceStorage(), _childData() {}

    ResourceId Insert(T&& item) override {
        
        ResourceId result = ResourceStorage::Insert(std::forward(item));

        if (_childData.size() <= result.index())
            _childData.resize(result.index() + 1);
    }

    ResourceId InsertChild(T&& item, ResourceId parent) {
        assert(_childData[parent.index()].parent == UINT32_MAX);
        
        ResourceId childId = Insert(std::forward(item));

        ChildData oldChildData = _childData[parent.index()];
        if (oldChildData.child != UINT32_MAX) {
            _childData[oldChildData.child].parent = childId.index();
        }
        oldChildData.parent = parent.index();
        _childData[childId.index()] = oldChildData;

        _childData[parent.index].child = childId.index();

        return childId;
    }

    bool TryRemove(ResourceId item) override {

        if (id.type() != type)
            return false;

        uint32_t generation = id.generation();
        uint32_t index = id.index();

        if (index >= _generations.size())
            return false;

        if (_generations[index] != generation)
            return false;

        if (!_storage[index].has_value())
            return false;

        // removing someones child
        ChildData* childData = &_childData[item.index()];
        if (childData->parent != UINT32_MAX) {
            
            RemoveUnchecked(item);

            if (childData->child != UINT32_MAX) {
                _childData[childData->child].parent = childData->parent;
            }

            _childData[childData->parent].child = childData->child;
            *childData = ChildData();
            
            return true;
        }

        // first removing all the children and then the parent
        while (childData->child != UINT32_MAX) {
            uint32_t childIndex = childData.child;
            
            RemoveUnchecked(childIndex);
            
            *childData = ChildData();
            childData = &_childData[childIndex];
        }
        *childData = ChildData();

        RemoveUnchecked(item);

        return true;
    }

    void clear() override {
        for (uint32_t i = 0; i < _storage.size(); i++) {
            if (!_storage[i].has_value())
                continue;

            ResourceId id = ResourceId::Compose(type, i, _generations[i]);
            TryRemove(id);
        }
        ResourceStorage::clear();
        _childData.clear();
    }
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
        return _storage->GetUnchecked(id);
    }

    T* try_get() {
        return _storage->TryGet(id);
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
        _storage->Replace(id, newValue);
    }

private: 
    ResourceStorage<T>* _storage; 
};

template<typename T>
using ResourceRefs = std::vector<ResourceRef<T>>;

