#pragma once
#include <framed_storage.h>
#include <object_pool.h>

template<typename T>
struct FramedObjectPool {

    void SetFrame(uint32_t currentFrame) {
        storage.SetFrame(currentFrame);
        storage->Reset();
        if (currentFrame != _currentFrame) {
            storage.get(_currentFrame).TransferAvailableTo(storage.get(currentFrame));
            _currentFrame = currentFrame;
        }
    }

    bool isEmpty() {
        return storage->isEmpty();
    }

    void Insert(T&& value) {
        storage->Insert(std::move(value));
    }

    T& Borrow() {
        return storage->BorrowAndForget();
    }

    void clear() {
        storage.clear();
    }

private: 
    uint32_t _currentFrame = 0;

    FramedStorage<ObjectPool<T>> storage;
};