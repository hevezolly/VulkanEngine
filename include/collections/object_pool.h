#pragma once
#include <common.h>

template<typename T>
struct Borrowed;

template<typename T>
struct API ObjectPool
{
    struct Node {
        Node* next;
        Node* previous;
        T item;

        Node(T&& movedItem): 
            next(nullptr), previous(nullptr), item(std::move(movedItem)) {}
    };

    bool isEmpty() {
        return nextAvailable == nullptr;
    }

    void Reset() {
        nextAvailable = end;
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
        Return(newNode);
    }

    ObjectPool(): begin(nullptr), end(nullptr), nextAvailable(nullptr) {
        _storage = new ObjectPool*;
        *_storage = this;
    }

    ~ObjectPool() {
        if (_storage == nullptr)
            return;

        delete _storage;

        Node* iterator = begin;
        while (iterator != nullptr) {
            Node* next = iterator->next;
            delete iterator;
            iterator = next;
        }
        
    }

    ObjectPool(const ObjectPool&) = delete; 
    
    ObjectPool& operator=(const ObjectPool&) = delete; 
    
    ObjectPool(ObjectPool&& other) noexcept {
        *this = std::move(other);
    }
    
    ObjectPool& operator=(ObjectPool&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        begin = other.begin;
        nextAvailable = other.nextAvailable;
        end = other.end;
        *_storage = this;
        other._storage = nullptr;
        other.begin = nullptr;
        other.end = nullptr;
        other.nextAvailable = nullptr;

        return *this;
    }

    void TransferAvailableTo(ObjectPool& other) {
        if (other._storage == _storage)
            return;
        
        if (nextAvailable == nullptr)
            return;

        Node* rangeStart = begin;
        Node* rangeEnd = nextAvailable;

        begin = nextAvailable->next;

        if (nextAvailable == end)
            end = nullptr;
        
        nextAvailable = nullptr;

        rangeEnd->next = other.begin;

        if (other.begin != nullptr)
            other.begin->previous = rangeEnd;

        rangeStart->previous = nullptr;
        other.begin = rangeStart;

        if (other.nextAvailable == nullptr)
            other.nextAvailable = rangeEnd;
        
        if (other.end == nullptr)
            other.end = rangeEnd;
    }

    friend Borrowed;
private:

    ObjectPool** _storage;
    Node* begin;
    Node* nextAvailable;
    Node* end;

    void Return(Node* node) {
        assert(node != nullptr);
        assert(node != nextAvailable);
        
        Node* prev = node->previous;
        Node* next = node->next;

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
};

template<typename T>
struct API Borrowed {
    
    T& val() {
        assert(_ptr != nullptr);
        return _ptr->item;
    }

    T* try_get() {
        assert(_ptr != nullptr);
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
        _storage = nullptr;
    }

    Borrowed(ObjectPool<T>** storage, typename ObjectPool<T>::Node* p): _storage(storage), _ptr(p){}

    ~Borrowed() {

        if (_storage == nullptr)
            return;

        (*_storage)->Return(_ptr);
    }

    Borrowed(const Borrowed&) = delete; 
    
    Borrowed& operator=(const Borrowed&) = delete; 
    
    Borrowed(Borrowed&& other) noexcept {
        _ptr = other._ptr;
        _storage = other._storage;
        other.Forget();
    }
    
    Borrowed& operator=(Borrowed&& other) noexcept {
        if (this != &other) {
            _ptr = other._ptr;
            _storage = other._storage;
            other.Forget();
        }

        return *this;
    }

private:
    typename ObjectPool<T>::Node* _ptr;
    ObjectPool<T>** _storage;
};

template<typename T>
Borrowed<T> ObjectPool<T>::Borrow() {
    assert(!isEmpty());

    Node* extracted = nextAvailable;
    nextAvailable = nextAvailable->previous;

    return Borrowed(_storage, extracted);
}