#pragma once

#include <common.h>

template<typename T>
struct FramedStorage {

    void SetFrame(uint32_t frame) {
        if (_storage.size() <= frame)
            _storage.resize(frame+1);

        currentFrame = frame;
    }

    std::vector<T>& operator*() {
        return _storage[currentFrame];
    }

    std::vector<T>* operator ->() {
        return &_storage[currentFrame];
    }

    std::vector<T>* operator &() {
        return &_storage[currentFrame];
    }

    void clearAll() {
        for (int i = 0; i < _storage.size(); i++) {
            _storage[i].clear();
        }
    }

    T& operator[](size_t i) {
        return _storage[currentFrame][i];
    }

private:
    uint32_t currentFrame = 0;
    std::vector<std::vector<T>> _storage;
};