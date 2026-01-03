#pragma once
#include <feature_set.h>

#define PREALLOCATED_SIZE 10240

enum struct AllocationType {
    Bump,
    Heap
};

template<typename T>
struct MemChunk {
    T* data;
    uint32_t size;
    AllocationType allocType;

    T& operator[](int index) {
        assert(index >= 0 && index < size);
        return *(data + index);
    }

    const T& operator[](int index) const {
        assert(index >= 0 && index < size);
        return *(data + index);
    }
    
    operator MemChunk<char>() {
        MemChunk<char> result;
        result.data = reinterpret_cast<char*>(data);
        result.size = size * sizeof(T);
        result.allocType = allocType;
        return result;
    }

    struct Iterator {
        // Iterator traits (required for STL compatibility pre-C++20)
        using iterator_category = std::forward_iterator_tag; //
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        // Constructor
        Iterator(pointer ptr) : m_ptr(ptr) {}

        // Required Operators
        // Dereference operator
        reference operator*() const { return *m_ptr; }
        pointer operator->() const { return m_ptr; }

        // Prefix increment operator (++it)
        Iterator& operator++() {
            m_ptr++;
            return *this;
        }

        // Postfix increment operator (it++)
        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        // Comparison operators
        friend bool operator==(const Iterator& a, const Iterator& b) {
            return a.m_ptr == b.m_ptr;
        }
        friend bool operator!=(const Iterator& a, const Iterator& b) {
            return a.m_ptr != b.m_ptr;
        }

    private:
        pointer m_ptr; // Pointer to the current element
    };

    Iterator begin() { return Iterator(data); }
    Iterator end() { return Iterator(data + size); } 
};

using RawMemChunk = MemChunk<char>;

struct Allocator: FeatureSet,
    CanHandle<InitMsg>,
    CanHandle<DestroyMsg>,
    CanHandle<BeginFrameMsg>
{

    Allocator(RenderContext& context, uint32_t preallocatedSize=PREALLOCATED_SIZE);

    virtual void OnMessage(InitMsg*);
    virtual void OnMessage(DestroyMsg*);
    virtual void OnMessage(BeginFrameMsg*);

    template<typename T>
    MemChunk<T> BumpAllocate(uint32_t count = 1) {
        assert(count > 0);
        size_t alignment = alignof(T);
        size_t allocationSize = sizeof(T) * count;
        uintptr_t alignedOffset = freeOffset + (alignment - freeOffset % alignment) % alignment; 
        assert(alignedOffset + allocationSize <= allocatedSize);

        freeOffset = alignedOffset + allocationSize;
        T* ptr = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(chunk)+ alignedOffset); 
        memset(ptr, 0, allocationSize);
        return MemChunk<T>{ptr, count, AllocationType::Bump};
    }

    template<typename T>
    MemChunk<T> HeapAllocate(uint32_t count = 1) {
        assert(count > 0);
        size_t alignment = alignof(T);
        size_t allocationSize = sizeof(T) * count;

        T* ptr = static_cast<T*>(malloc(allocatedSize));
        return MemChunk<T>{ptr, count, AllocationType::Heap};
    }

    template<typename T>
    void Free(MemChunk<T> borrowed, bool force = false) {

        if (borrowed.allocType == AllocationType::Heap) {
            free(borrowed.data);
            return;
        }

        uintptr_t b = reinterpret_cast<uintptr_t>(borrowed.data);
        uintptr_t c = reinterpret_cast<uintptr_t>(chunk);
        assert(b >= c);
        uintptr_t chunkStart = b - c;

        if (force || (sizeof(T) * borrowed.size + chunkStart == freeOffset))
            freeOffset = std::min(chunkStart, freeOffset);
    }

private:

    char* chunk;
    size_t allocatedSize;
    uintptr_t freeOffset;


};