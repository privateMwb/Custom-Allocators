#pragma once

#include <cstddef>
#include <new>
#include <utility>

class FreeListAllocator {
public:
    // Debug Statistics
    struct Stats {
        std::size_t totalAllocated = 0;
        std::size_t currentUsed    = 0;
        std::size_t peakUsed       = 0;
        std::size_t allocations    = 0;
        std::size_t deallocations  = 0;
    };

private:
    // Free Block
    struct FreeBlock {
        std::size_t size;
        FreeBlock*  next;
    };

    // Allocation Header
    struct AllocationHeader {
        std::size_t size;
        std::size_t adjustment;
    };

    // Core Memory
    std::byte* memory;

    // Configuration
    std::size_t cap;
    std::size_t alignment;
    std::size_t usedMemory;

    // Free List
    FreeBlock* freeList;

    // Debug Stats
    Stats stats;

    // Alignment Utilities
    [[nodiscard]] static constexpr std::size_t alignForward(std::size_t ptr, std::size_t alignment) noexcept;
    [[nodiscard]] static constexpr bool isPowerOfTwo(std::size_t value) noexcept;

    // Free List Management
    void coalesce() noexcept;

public:
    // Constructors & Destructor
    explicit FreeListAllocator(std::size_t size, std::size_t alignment = alignof(std::max_align_t));
    ~FreeListAllocator();

    FreeListAllocator(const FreeListAllocator&)            = delete;
    FreeListAllocator& operator=(const FreeListAllocator&) = delete;

    FreeListAllocator(FreeListAllocator&& other) noexcept;
    FreeListAllocator& operator=(FreeListAllocator&& other) noexcept;

    // Memory Management
    [[nodiscard]] void* allocate(std::size_t size, std::size_t request_alignment = alignof(std::max_align_t)) noexcept;

    void deallocate(void* ptr) noexcept;

    // Object Lifecycle
    template<typename T, typename... Args>
    [[nodiscard]] T* create(Args&&... args);

    template<typename T>
    void destroy(T* ptr);

    // Introspection
    [[nodiscard]] bool owns(const void* ptr) const noexcept;

    [[nodiscard]] const Stats& getStats() const noexcept;

    [[nodiscard]] std::size_t used()      const noexcept;
    [[nodiscard]] std::size_t remaining() const noexcept;
    [[nodiscard]] std::size_t capacity()  const noexcept;
};

#include "FreeListAllocator.tpp"