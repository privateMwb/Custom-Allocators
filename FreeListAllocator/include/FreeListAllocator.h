#pragma once 

#include <cstddef>
#include <new>
#include <utility>

using std::byte;
using std::size_t;

class FreeListAllocator {
    private:
    // Free Block
    struct FreeBlock {
        size_t size;
        FreeBlock* next;
    };
    
    // Allocation Header
    struct AllocationHeader {
        size_t size;
        size_t adjustment;
    };
    
    // Core Memory
    byte* memory;
    
    // Configuration
    size_t cap;
    size_t alignment;
    size_t usedMemory;
    
    // Free List
    FreeBlock* freeList;
    
    // Alignment Utilities
    static size_t alignForward(size_t ptr, size_t alignment);
    
    // Free List Management
    void coalesce();
    
    public:
    // Constructors & Destructor
    explicit FreeListAllocator(size_t size, size_t alignment = alignof(std::max_align_t));
    ~FreeListAllocator();
    
    FreeListAllocator(const FreeListAllocator&) = delete;
    FreeListAllocator& operator=(const FreeListAllocator&) = delete;
    
    FreeListAllocator(FreeListAllocator&& other) noexcept;
    FreeListAllocator& operator=(FreeListAllocator&& other) noexcept;
    
    // Memory Management
    void* allocate(size_t size, size_t request_alignment = alignof(std::max_align_t));
    
    void deallocate(void* ptr);
    
    // Object Lifecycle
    template<typename T, typename... Args>
    T* create(Args&&... args);
    
    template<typename T>
    void destroy(T* ptr);
    
    // Debug / Safety
    bool owns(void* ptr) const noexcept;
    
    // Capacity
    size_t used() const noexcept;
    size_t remaining() const noexcept;
    size_t capacity() const noexcept;
};

#include "FreeListAllocator.tpp"


