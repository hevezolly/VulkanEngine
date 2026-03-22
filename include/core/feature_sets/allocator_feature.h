#pragma once
#include <feature_set.h>

#define PREALLOCATED_SIZE 10240

struct Allocator;

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
        ASSERT(index >= 0 && index < size);
        return *(data + index);
    }

    const T& operator[](int index) const {
        ASSERT(index >= 0 && index < size);
        return *(data + index);
    }

    void clearToZero() {
        if (data != nullptr)
            memset(data, 0, size * sizeof(T));
    }
    
    operator MemChunk<char>() {
        MemChunk<char> result;
        result.data = reinterpret_cast<char*>(data);
        result.size = size * sizeof(T);
        result.allocType = allocType;
        return result;
    }

    static constexpr MemChunk Null() {
        return {
            nullptr,
            0,
            AllocationType::Heap
        };
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

template<typename T>
struct MemBuffer {

    inline uint32_t size() {return _size;}
    inline uint32_t capacity() {return _data.size;}

    inline T* data() {return _data.data;}

    T& operator[](int index) {
        ASSERT(index >= 0 && index < _size);
        return _data[index];
    }

    const T& operator[](int index) const {
        ASSERT(index >= 0 && index < _size);
        return _data[index];
    }

    T& back() {
        ASSERT(_size > 0);
        return _data[_size - 1];
    }

    const T& back() const {
        ASSERT(_size > 0);
        return _data[_size - 1];
    }

    void push_back(const T& value) {
        ASSERT(_size < _data.size);

        _data[_size++] = value;
    }

    void push_back(T&& value) {
        ASSERT(_size < _data.size);

        _data[_size++] = std::move(value);
    }

    void pop_back() {
        ASSERT(_size > 0);
        _size--;
    }

    bool contains(const T& item) {
        for (int i = 0; i < _size; i++) {
            if (_data[i] == item)
                return true;
        }
        return false;
    }

    void fast_delete_at(uint32_t index) {
        ASSERT(_size > 0);
        ASSERT(index < _size);
        if (index != _size-1) {
            std::swap(_data[_size-1], _data[index]);
        }
        pop_back();
    }

    void fast_delete(const T& item) {
        uint32_t index = 0;
        for (; index < _size; index++) {
            if (_data[index] == item)
                break;
        }

        if (index < _size)
            fast_delete_at(index);
    }

    void resize(uint32_t count) {
        _size = count;
    }

    MemBuffer(MemChunk<T> data): _size(0), _data(data) {}
    
    explicit operator MemChunk<T>() const {
        return _data;
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

    Iterator begin() { return Iterator(data()); }
    Iterator end() { return Iterator(data() + size()); } 
    
private:
    MemChunk<T> _data;
    uint32_t _size;
};

using RawMemChunk = MemChunk<char>;


struct API ArenaContext {

    ArenaContext(Allocator* a, uintptr_t offset) : freeOffset(offset), alloc(a){}

    RULE_5(ArenaContext)
private:
    Allocator* alloc;
    uintptr_t freeOffset;
};

struct API Allocator: FeatureSet,
    CanHandle<EarlyInitMsg>,
    CanHandle<DestroyMsg>,
    CanHandle<BeginFrameMsg>
{

    Allocator(RenderContext& context, uint32_t preallocatedSize=PREALLOCATED_SIZE);

    virtual void OnMessage(EarlyInitMsg*);
    virtual void OnMessage(DestroyMsg*);
    virtual void OnMessage(BeginFrameMsg*);

    template<typename T>
    MemChunk<T> BumpAllocate(uint32_t count = 1) {
        if (count == 0)
            return MemChunk<T>::Null();
        size_t alignment = alignof(T);
        size_t allocationSize = sizeof(T) * count;
        uintptr_t alignedOffset = freeOffset + (alignment - freeOffset % alignment) % alignment;
        ASSERT(alignedOffset + allocationSize <= allocatedSize);

        freeOffset = alignedOffset + allocationSize;
        T* ptr = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(chunk)+ alignedOffset); 

        auto result = MemChunk<T>{ptr, count, AllocationType::Bump};
        result.clearToZero();
        return result;
    }

    template<typename T>
    MemChunk<T> HeapAllocate(uint32_t count = 1) {
        if (count == 0)
            return MemChunk<T>::Null();
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
        ASSERT(b >= c);
        uintptr_t chunkStart = b - c;

        if (force || (sizeof(T) * borrowed.size + chunkStart == freeOffset))
            freeOffset = std::min(chunkStart, freeOffset);
    }

    template<typename T>
    void Free(MemBuffer<T> borrowed, bool force = false) {
        Free(static_cast<MemChunk<T>>(borrowed), force);
    }

    ArenaContext BeginContext() {
        return ArenaContext(this, freeOffset);
    }

    friend ArenaContext;

private:

    void SetOffset(uintptr_t offset) {
        freeOffset = offset;
    }

    char* chunk;
    size_t allocatedSize;
    uintptr_t freeOffset;
};