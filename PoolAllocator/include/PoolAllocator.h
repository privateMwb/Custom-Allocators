#pragma once 

#include <cstddef>
#include <new>
#include <utility>

using std::size_t;
using std::byte;

class PoolAllocator {
    private:
    struct FreeNode {
        FreeNode* next;
    };
    
    // Core Memory;
    byte* memory;
    
    // Pool Configuration
    size_t blockSize;
    size_t stride;
    size_t blockCount;
    size_t alignment;
    
    // Usage Tracking
    size_t freeBlockCount;
    
    // Free List
    FreeNode* freeList;
    
    // Alignment Utilities
    static size_t alignForward(size_t ptr, size_t alignment);
    
    public:
    // Constructors & Destructor
    explicit PoolAllocator(size_t blockSize, size_t blockCount, size_t alignment = alignof(std::max_align_t));
    ~PoolAllocator();
    
    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;
    
    PoolAllocator(PoolAllocator&& other) noexcept;
    PoolAllocator& operator=(PoolAllocator&& other) noexcept;
    
    // Memory Management
    void* allocate();
    void deallocate(void* ptr);
    
    // Object Lifecycle
    template<typename T, typename... Args>
    T* create(Args&&... args);
    
    template<typename T>
    void destroy(T* ptr);
    
    // Debug / Safety
    bool owns(void* ptr) const noexcept;
    
    // Capacity
    size_t capacity() const noexcept;
    size_t usedBlocks() const noexcept;
    size_t freeBlocks() const noexcept;
    size_t totalBlocks() const noexcept;
    size_t block_size() const noexcept;
};

#include "PoolAllocator.tpp"






