#pragma once

#include <cstddef>
#include <new>
#include <utility>

using std::size_t;
using std::byte;

class StackAllocator {
    private:
    // Core Memory
    byte* memory;
    
    // Stack Configuration
    size_t cap;
    size_t offset;
    size_t alignment;
    
    // Alignment Utilities
    static size_t alignForward(size_t ptr, size_t alignment);
    
    public:
    // Constructors & Destructor
    explicit StackAllocator(size_t size, size_t alignment = alignof(std::max_align_t));
    
    ~StackAllocator();
    
    StackAllocator(const StackAllocator&) = delete;
    StackAllocator& operator=(const StackAllocator&) = delete;
    
    StackAllocator(StackAllocator&& other) noexcept;
    StackAllocator& operator=(StackAllocator&& other) noexcept;
    
    // Memory Management
    void* allocate(size_t size, size_t request_alignment = alignof(std::max_align_t));
    
    size_t getMarker() const noexcept;
    void freeToMarker(size_t marker);
    
    // Object Lifecycle
    template<typename T, typename... Args>
    T* create(Args&&... args);
    
    template<typename T>
    void destroy(T* ptr);
    
    // Debug / Safety
    bool owns(void* ptr) const noexcept;
    
    // State Management
    void clear();
    
    // Capacity
    size_t used() const noexcept;
    size_t remaining() const noexcept;
    size_t capacity() const noexcept;
};

#include "StackAllocator.tpp"