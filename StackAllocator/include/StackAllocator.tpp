#include <cassert>

// Alignment Utilities
constexpr bool StackAllocator::isPowerOfTwo(std::size_t value) noexcept {
    return value != 0 && (value & (value - 1)) == 0;
}

constexpr std::size_t StackAllocator::alignForward(std::size_t ptr, std::size_t alignment) noexcept {
    return (ptr + alignment - 1) & ~(alignment - 1);
}

// Constructors & Destructor
inline StackAllocator::StackAllocator(std::size_t size, std::size_t alignment)
    : memory(static_cast<std::byte*>(::operator new(size, std::align_val_t(alignment))))
    , cap(size)
    , offset(0)
    , alignment(alignment)
    , stats{}
{
    assert(isPowerOfTwo(alignment) && "alignment must be a non-zero power of two");
    assert(size > 0                && "size must be > 0");
}

inline StackAllocator::~StackAllocator() {
    ::operator delete(memory, std::align_val_t(alignment));
}

inline StackAllocator::StackAllocator(StackAllocator&& other) noexcept
    : memory(other.memory)
    , cap(other.cap)
    , offset(other.offset)
    , alignment(other.alignment)
    , stats(other.stats)
{
    other.memory    = nullptr;
    other.cap       = 0;
    other.offset    = 0;
    other.alignment = 0;
    other.stats     = {};
}

inline StackAllocator& StackAllocator::operator=(StackAllocator&& other) noexcept {
    if (this != &other) {
        ::operator delete(memory, std::align_val_t(alignment));

        memory    = other.memory;
        cap       = other.cap;
        offset    = other.offset;
        alignment = other.alignment;
        stats     = other.stats;

        other.memory    = nullptr;
        other.cap       = 0;
        other.offset    = 0;
        other.alignment = 0;
        other.stats     = {};
    }

    return *this;
}

// Memory Management
inline void* StackAllocator::allocate(std::size_t size, std::size_t request_alignment) noexcept {
    assert(isPowerOfTwo(request_alignment)  && "request_alignment must be a non-zero power of two");
    assert(request_alignment <= alignment   && "request_alignment exceeds allocator base alignment");

    std::size_t aligned    = alignForward(offset, request_alignment);
    std::size_t adjustment = aligned - offset;

    if (offset + adjustment + size > cap) [[unlikely]]
        return nullptr;

    void* ptr = memory + aligned;
    offset    = aligned + size;

    ++stats.allocations;
    stats.currentUsed = offset;
    stats.totalAllocated += adjustment + size;
    if (offset > stats.peakUsed) [[unlikely]]
        stats.peakUsed = offset;

    return ptr;
}
inline std::size_t StackAllocator::getMarker() const noexcept {
    return offset;
}

inline void StackAllocator::freeToMarker(std::size_t marker) noexcept {
    assert(marker <= offset && "marker exceeds current stack offset");

    offset            = marker;
    stats.currentUsed = offset;
}

// Object Lifecycle
template<typename T, typename... Args>
T* StackAllocator::create(Args&&... args) {
    void* raw = allocate(sizeof(T), alignof(T));

    if (!raw) [[unlikely]]
        return nullptr;

    return ::new (raw) T(std::forward<Args>(args)...);
}

template<typename T>
void StackAllocator::destroy(T* ptr) noexcept {
    if (!ptr || !owns(ptr)) [[unlikely]] return;

    ptr->~T();
}

// State Management
inline void StackAllocator::reset() noexcept {
    offset            = 0;
    stats.currentUsed = 0;
}

// Introspection
inline bool StackAllocator::owns(const void* ptr) const noexcept {
    const auto* address = static_cast<const std::byte*>(ptr);
    const auto* start   = memory;
    const auto* end     = memory + cap;

    return address >= start && address < end;
}

inline const StackAllocator::Stats& StackAllocator::getStats() const noexcept {
    return stats;
}

inline std::size_t StackAllocator::used() const noexcept {
    return offset;
}

inline std::size_t StackAllocator::remaining() const noexcept {
    return cap - offset;
}

inline std::size_t StackAllocator::capacity() const noexcept {
    return cap;
}