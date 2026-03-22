#pragma once
#include <common.h>

template<typename T>
struct Borrowed;

template<typename T>
struct ObjectPoolData;

template<typename T>
struct ObjectPool
{
    struct Node {
        Node* next;
        Node* previous;
        T item;

        Node(T&& movedItem): 
            next(nullptr), previous(nullptr), item(std::move(movedItem)) {}
    };

    bool isEmpty() {
        return !_data || _data->nextAvailable == nullptr;
    }

    void Reset() {
        _data->nextAvailable = _data->end;
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
        _data->Return(newNode);
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
        
        if (_data->nextAvailable == nullptr)
            return;

        Node* rangeStart = _data->begin;
        Node* rangeEnd = _data->nextAvailable;

        _data->begin = _data->nextAvailable->next;

        if (_data->nextAvailable == _data->end)
            _data->end = nullptr;
        
        _data->nextAvailable = nullptr;

        rangeEnd->next = other._data->begin;

        if (other._data->begin != nullptr)
            other._data->begin->previous = rangeEnd;

        rangeStart->previous = nullptr;
        other._data->begin = rangeStart;

        if (other._data->nextAvailable == nullptr)
            other._data->nextAvailable = rangeEnd;
        
        if (other._data->end == nullptr)
            other._data->end = rangeEnd;
    }

    friend Borrowed;
private:

    std::shared_ptr<ObjectPoolData<T>> _data;
};

template<typename T>
struct ObjectPoolData {

    typename ObjectPool<T>::Node* begin;
    typename ObjectPool<T>::Node* nextAvailable;
    typename ObjectPool<T>::Node* end;

    void Return(typename ObjectPool<T>::Node* node) {
        ASSERT(node != nullptr);
        ASSERT(node != nextAvailable);
        
        typename ObjectPool<T>::Node* prev = node->previous;
        typename ObjectPool<T>::Node* next = node->next;

        if (node == end)
            end = prev;

        if (prev != nullptr)
            prev->next = next;
        
        if (next != nullptr)
            next->previous = prev;

        node->next = begin;

        if (begin != nullptr)
            begin->previous = node;

        node->previous = nullptr;
        begin = node;

        if (nextAvailable == nullptr) {
            nextAvailable = node;
        }

        if (end == nullptr)
            end = begin;
    }

    ObjectPoolData(): begin(nullptr), end(nullptr), nextAvailable(nullptr){}

    ObjectPoolData(const ObjectPoolData&) = delete; 
    
    ObjectPoolData& operator=(const ObjectPoolData&) = delete;

    ObjectPoolData(ObjectPoolData&&) = delete; 
    
    ObjectPoolData& operator=(ObjectPoolData&&) = delete;

    ~ObjectPoolData() {
        auto* iterator = begin;
        while (iterator != nullptr) {
            auto* next = iterator->next;
            delete iterator;
            iterator = next;
        }
    }
};

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

    void Forget() {
        _ptr = nullptr;
        _data.reset();
    }

    Borrowed(std::shared_ptr<ObjectPoolData<T>> storage, typename ObjectPool<T>::Node* p): 
        _data(storage), _ptr(p){}

    ~Borrowed() {

        if (_data == nullptr)
            return;

        _data->Return(_ptr);
    }

    Borrowed(const Borrowed&) = delete; 
    
    Borrowed& operator=(const Borrowed&) = delete; 
    
    Borrowed(Borrowed&& other) noexcept {
        _ptr = other._ptr;
        _data = std::move(other._data);
        other.Forget();
    }
    
    Borrowed& operator=(Borrowed&& other) noexcept {
        if (this != &other) {
            _ptr = other._ptr;
            _data = std::move(other._data);
            other.Forget();
        }

        return *this;
    }

    void MoveTo(ObjectPool<T>& other) {
        
        auto* prev = _ptr->previous;
        auto* next = _ptr->next;

        if (_data->nextAvailable == _ptr) {
            _data->nextAvailable = prev;
        }

        if (_data->begin == _ptr) {
            _data->begin = next;
        }

        if (_data->end == _ptr) {
            _data->end = prev;
        }

        if (prev != nullptr) {
            prev->next = next;
        }

        if (next != nullptr) {
            next->previous = prev;
        }

        _data = other._data;

        _ptr->next = nullptr;

        if (_data->end != nullptr) {
            _data->end->next = _ptr;
        }
        else {
            ASSERT(_data->nextAvailable == nullptr);
            _data->begin = _ptr;
        }

        _ptr->previous = _data->end;
        _data->end = _ptr;
    }

private:
    typename ObjectPool<T>::Node* _ptr;
    std::shared_ptr<ObjectPoolData<T>> _data;
};

template<typename T>
inline Borrowed<T> ObjectPool<T>::Borrow() {
    ASSERT(!isEmpty());

    Node* extracted = _data->nextAvailable;
    _data->nextAvailable = _data->nextAvailable->previous;

    return Borrowed<T>(_data, extracted);
}