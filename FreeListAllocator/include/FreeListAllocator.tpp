#include <cassert>
#include <cstdint>

// Alignment Utilities
constexpr bool FreeListAllocator::isPowerOfTwo(std::size_t value) noexcept {
    return value != 0 && (value & (value - 1)) == 0;
}

constexpr std::size_t FreeListAllocator::alignForward(std::size_t ptr, std::size_t alignment) noexcept {
    return (ptr + alignment - 1) & ~(alignment - 1);
}

// Free List Management
inline void FreeListAllocator::coalesce() noexcept {
    FreeBlock* current = freeList;

    while (current && current->next) {
        std::byte* currentEnd = reinterpret_cast<std::byte*>(current) + current->size;
        std::byte* nextStart  = reinterpret_cast<std::byte*>(current->next);

        if (currentEnd == nextStart) {
            current->size += current->next->size;
            current->next  = current->next->next;
        } else {
            current = current->next;
        }
    }
}

// Constructors & Destructor
inline FreeListAllocator::FreeListAllocator(std::size_t size, std::size_t alignment)
    : memory(static_cast<std::byte*>(::operator new(size, std::align_val_t(alignment))))
    , cap(size)
    , alignment(alignment)
    , usedMemory(0)
    , freeList(reinterpret_cast<FreeBlock*>(memory))
    , stats{}
{
    assert(isPowerOfTwo(alignment) && "alignment must be a non-zero power of two");
    assert(size > 0                && "size must be > 0");

    freeList->size = cap;
    freeList->next = nullptr;
}

inline FreeListAllocator::~FreeListAllocator() {
    ::operator delete(memory, std::align_val_t(alignment));
}

inline FreeListAllocator::FreeListAllocator(FreeListAllocator&& other) noexcept
    : memory(other.memory)
    , cap(other.cap)
    , alignment(other.alignment)
    , usedMemory(other.usedMemory)
    , freeList(other.freeList)
    , stats(other.stats)
{
    other.memory     = nullptr;
    other.cap        = 0;
    other.alignment  = 0;
    other.usedMemory = 0;
    other.freeList   = nullptr;
    other.stats      = {};
}

inline FreeListAllocator& FreeListAllocator::operator=(FreeListAllocator&& other) noexcept {
    if (this != &other) {
        ::operator delete(memory, std::align_val_t(alignment));

        memory     = other.memory;
        cap        = other.cap;
        alignment  = other.alignment;
        usedMemory = other.usedMemory;
        freeList   = other.freeList;
        stats      = other.stats;

        other.memory     = nullptr;
        other.cap        = 0;
        other.alignment  = 0;
        other.usedMemory = 0;
        other.freeList   = nullptr;
        other.stats      = {};
    }

    return *this;
}

// Memory Management
inline void* FreeListAllocator::allocate(std::size_t size, std::size_t request_alignment) noexcept {
    assert(isPowerOfTwo(request_alignment)  && "request_alignment must be a non-zero power of two");
    assert(request_alignment <= alignment   && "request_alignment exceeds allocator base alignment");

    if (!freeList) [[unlikely]]
        return nullptr;

    FreeBlock* prev    = nullptr;
    FreeBlock* current = freeList;

    std::size_t    adjustment    = 0;
    std::size_t    requiredSize  = 0;
    std::uintptr_t alignedAddress = 0;

    while (current) {
        std::uintptr_t currentAddress = reinterpret_cast<std::uintptr_t>(current);

        alignedAddress = alignForward(currentAddress + sizeof(AllocationHeader), request_alignment);
        adjustment     = alignedAddress - currentAddress;
        requiredSize   = adjustment + size;

        if (current->size >= requiredSize) break;

        prev    = current;
        current = current->next;
    }

    if (!current) [[unlikely]] {
        coalesce();

        prev    = nullptr;
        current = freeList;

        while (current) {
            std::uintptr_t currentAddress = reinterpret_cast<std::uintptr_t>(current);

            alignedAddress = alignForward(currentAddress + sizeof(AllocationHeader), request_alignment);
            adjustment     = alignedAddress - currentAddress;
            requiredSize   = adjustment + size;

            if (current->size >= requiredSize) break;

            prev    = current;
            current = current->next;
        }

        if (!current) [[unlikely]]
            return nullptr;
    }

    std::size_t remainingSize = current->size - requiredSize;

    if (remainingSize >= sizeof(FreeBlock)) {
        FreeBlock* nextBlock = reinterpret_cast<FreeBlock*>(
            reinterpret_cast<std::byte*>(current) + requiredSize);

        nextBlock->size = remainingSize;
        nextBlock->next = current->next;

        if (prev == nullptr) freeList   = nextBlock;
        else                 prev->next = nextBlock;
    } else {
        requiredSize = current->size;

        if (prev == nullptr) freeList   = current->next;
        else                 prev->next = current->next;
    }

    AllocationHeader* header = reinterpret_cast<AllocationHeader*>(
        alignedAddress - sizeof(AllocationHeader));

    header->size       = requiredSize;
    header->adjustment = adjustment;

    usedMemory          += requiredSize;
    stats.currentUsed    = usedMemory;
    stats.totalAllocated += requiredSize;

    ++stats.allocations;

    if (usedMemory > stats.peakUsed) [[unlikely]]
        stats.peakUsed = usedMemory;

    return reinterpret_cast<void*>(alignedAddress);
}

inline void FreeListAllocator::deallocate(void* ptr) noexcept {
    if (!ptr || !owns(ptr)) [[unlikely]] return;

    std::byte* bytePtr = reinterpret_cast<std::byte*>(ptr);

    AllocationHeader* header = reinterpret_cast<AllocationHeader*>(
        bytePtr - sizeof(AllocationHeader));

    std::byte* startBlock = bytePtr - header->adjustment;

    FreeBlock* block = reinterpret_cast<FreeBlock*>(startBlock);
    block->size      = header->size;

    if (freeList == nullptr) {
        freeList    = block;
        block->next = nullptr;
    } else {
        FreeBlock* prev    = nullptr;
        FreeBlock* current = freeList;

        while (current && current < block) {
            prev    = current;
            current = current->next;
        }

        if (prev == nullptr) {
            block->next = freeList;
            freeList    = block;
        } else {
            block->next = current;
            prev->next  = block;
        }
    }

    usedMemory        -= header->size;
    stats.currentUsed  = usedMemory;

    ++stats.deallocations;
}

// Object Lifecycle
template<typename T, typename... Args>
T* FreeListAllocator::create(Args&&... args) {
    void* raw = allocate(sizeof(T), alignof(T));

    if (!raw) [[unlikely]]
        return nullptr;

    return ::new (raw) T(std::forward<Args>(args)...);
}

template<typename T>
void FreeListAllocator::destroy(T* ptr) {
    if (!ptr || !owns(ptr)) [[unlikely]] return;

    ptr->~T();
    deallocate(ptr);
}

// Introspection
inline bool FreeListAllocator::owns(const void* ptr) const noexcept {
    const auto* address = static_cast<const std::byte*>(ptr);
    const auto* start   = memory;
    const auto* end     = memory + cap;

    return address >= start && address < end;
}

inline const FreeListAllocator::Stats& FreeListAllocator::getStats() const noexcept {
    return stats;
}

inline std::size_t FreeListAllocator::used() const noexcept {
    return usedMemory;
}

inline std::size_t FreeListAllocator::remaining() const noexcept {
    return cap - usedMemory;
}

inline std::size_t FreeListAllocator::capacity() const noexcept {
    return cap;
}