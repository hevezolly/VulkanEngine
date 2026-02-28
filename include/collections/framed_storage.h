#pragma once

#include <common.h>

template<typename T>
struct FramedStorage {

    FramedStorage() {
        SetFrame(0);
    }

    void SetFrame(uint32_t frame) {
        if (_storage.size() <= frame) {
            _storage.resize(frame+1);
        }

        currentFrame = frame;
    }

    T& val() {
        return _storage[currentFrame];
    }

    T& operator*() {
        return _storage[currentFrame];
    }

    T* operator ->() {
        return &_storage[currentFrame];
    }

    T* operator &() {
        return &_storage[currentFrame];
    }

    T& get(uint32_t frame) {
        return _storage[frame];
    }

    uint32_t size() {
        return _storage.size();
    }

    void clear() {
        _storage.clear();
    }

    inline uint32_t getCurrentFrame() {
        return currentFrame;
    }

private:
    uint32_t currentFrame = 0;
    std::vector<T> _storage;
};