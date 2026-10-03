#pragma once
#include <common.h>

template<typename T>
struct Borrowed;

template<typename T>
struct WeakNodeRef;

template<typename T>
struct ObjectPoolData;

// Link part of a pool node. Pools keep their nodes in a circular list:
//   head <-> [available...] <-> divider <-> [borrowed...] <-> head
// head and divider are never unlinked by node operations, so a node can be
// unlinked without knowing which pool it belongs to.
struct ObjectPoolLink {
    ObjectPoolLink* next = this;
    ObjectPoolLink* previous = this;

    void Unlink() {
        previous->next = next;
        next->previous = previous;
        next = this;
        previous = this;
    }

    void InsertBefore(ObjectPoolLink* position) {
        previous = position->previous;
        next = position;
        position->previous->next = this;
        position->previous = this;
    }
};

template<typename T>
struct ObjectPool
{
    struct Node : ObjectPoolLink {
        T item;

        Node(T&& movedItem): item(std::move(movedItem)) {}
    };

    bool isEmpty() {
        return !_data || _data->divider.previous == &_data->head;
    }

    // Makes every node available again, including the ones handed out by BorrowAndForget.
    void Reset() {
        // A live Borrowed handle would end up holding a node that the pool considers available.
        ASSERT(_data->borrowedCount == 0);

        _data->divider.Unlink();
        _data->divider.InsertBefore(&_data->head);
    }

    Borrowed<T> Borrow();

    T& BorrowAndForget() {
        Borrowed<T> borrowed = Borrow();
        T& item = *borrowed;
        borrowed.Forget();
        return item;
    }

    void Insert(T&& value) {
        Node* newNode = new Node(std::move(value));
        newNode->InsertBefore(_data->head.next);
    }

    ObjectPool() {
        _data = std::make_shared<ObjectPoolData<T>>();
    }

    ~ObjectPool() = default;

    ObjectPool(const ObjectPool&) = default;

    ObjectPool& operator=(const ObjectPool&) = default;

    ObjectPool(ObjectPool&&) = default;

    ObjectPool& operator=(ObjectPool&&) = default;

    void TransferAvailableTo(ObjectPool& other) {
        if (other._data == _data)
            return;

        // A Borrowed handle returns into the pool it came from, so its node must not change pools.
        ASSERT(_data->borrowedCount == 0);

        if (isEmpty())
            return;

        ObjectPoolLink* rangeStart = _data->head.next;
        ObjectPoolLink* rangeEnd = _data->divider.previous;

        _data->head.next = &_data->divider;
        _data->divider.previous = &_data->head;

        ObjectPoolLink* otherStart = other._data->head.next;

        rangeStart->previous = &other._data->head;
        other._data->head.next = rangeStart;

        rangeEnd->next = otherStart;
        otherStart->previous = rangeEnd;
    }

    friend struct Borrowed<T>;
    friend struct WeakNodeRef<T>;
private:

    std::shared_ptr<ObjectPoolData<T>> _data;
};

template<typename T>
struct ObjectPoolData {

    ObjectPoolLink head;
    ObjectPoolLink divider;

    // Number of live Borrowed handles taken from this pool.
    uint32_t borrowedCount = 0;

    void Return(typename ObjectPool<T>::Node* node) {
        ASSERT(node != nullptr);
        ASSERT(borrowedCount > 0);

        node->Unlink();
        node->InsertBefore(head.next);
        borrowedCount--;
    }

    ObjectPoolData() {
        divider.InsertBefore(&head);
    }

    ObjectPoolData(const ObjectPoolData&) = delete;

    ObjectPoolData& operator=(const ObjectPoolData&) = delete;

    ObjectPoolData(ObjectPoolData&&) = delete;

    ObjectPoolData& operator=(ObjectPoolData&&) = delete;

    ~ObjectPoolData() {
        ObjectPoolLink* iterator = head.next;
        while (iterator != &head) {
            ObjectPoolLink* next = iterator->next;
            if (iterator != &divider)
                delete static_cast<typename ObjectPool<T>::Node*>(iterator);
            iterator = next;
        }
    }
};

// Owning handle: returns the item to its pool when destroyed.
template<typename T>
struct Borrowed {

    T& val() {
        ASSERT(_ptr != nullptr);
        return _ptr->item;
    }

    T* try_get() {
        ASSERT(_ptr != nullptr);
        return &(_ptr->item);
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

    // Gives up ownership without returning the item. It stays borrowed until the pool is reset.
    void Forget() {
        if (_data != nullptr)
            _data->borrowedCount--;

        _ptr = nullptr;
        _data.reset();
    }

    Borrowed(std::shared_ptr<ObjectPoolData<T>> storage, typename ObjectPool<T>::Node* p):
        _data(storage), _ptr(p){}

    ~Borrowed() {
        Release();
    }

    Borrowed(const Borrowed&) = delete;

    Borrowed& operator=(const Borrowed&) = delete;

    Borrowed(Borrowed&& other) noexcept {
        _ptr = other._ptr;
        _data = std::move(other._data);
        other._ptr = nullptr;
        other._data.reset();
    }

    Borrowed& operator=(Borrowed&& other) noexcept {
        if (this != &other) {
            Release();
            _ptr = other._ptr;
            _data = std::move(other._data);
            other._ptr = nullptr;
            other._data.reset();
        }

        return *this;
    }

    friend struct WeakNodeRef<T>;
private:

    void Release() {
        if (_data == nullptr)
            return;

        _data->Return(_ptr);
        _ptr = nullptr;
        _data.reset();
    }

    typename ObjectPool<T>::Node* _ptr;
    std::shared_ptr<ObjectPoolData<T>> _data;
};

// Non-owning reference to a pool node: never returns the item.
// The node can be moved between pools, and stays valid while its pool is alive.
// After the owning pool is reset, the node may be borrowed again by someone else.
template<typename T>
struct WeakNodeRef {

    T& val() {
        ASSERT(_ptr != nullptr);
        return _ptr->item;
    }

    T& operator*() {
        return val();
    }

    T* operator ->() {
        return &val();
    }

    // Moves the node to the borrowed part of the other pool,
    // taking it out of the available range if a reset put it there.
    void MoveTo(ObjectPool<T>& other) {
        ASSERT(_ptr != nullptr);
        _ptr->Unlink();
        _ptr->InsertBefore(&other._data->head);
    }

    WeakNodeRef(): _ptr(nullptr) {}

    // Takes the node from a Borrowed handle without returning it.
    WeakNodeRef(Borrowed<T>&& borrowed): _ptr(borrowed._ptr) {
        borrowed.Forget();
    }

private:
    typename ObjectPool<T>::Node* _ptr;
};

template<typename T>
inline Borrowed<T> ObjectPool<T>::Borrow() {
    ASSERT(!isEmpty());

    Node* extracted = static_cast<Node*>(_data->divider.previous);
    _data->divider.Unlink();
    _data->divider.InsertBefore(extracted);
    _data->borrowedCount++;

    return Borrowed<T>(_data, extracted);
}
