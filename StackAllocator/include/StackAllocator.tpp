// Alignment Utilities
inline size_t StackAllocator::alignForward(size_t ptr, size_t alignment) {
    return (ptr + alignment - 1) & ~(alignment - 1);
}

// Constructors & Destructor
inline StackAllocator::StackAllocator(size_t size, size_t alignment) : 
memory(static_cast<byte*>(::operator new(size, std::align_val_t(alignment)))),
cap(size),
offset(0),
alignment(alignment) {}

inline StackAllocator::~StackAllocator() {
    ::operator delete(memory, std::align_val_t(alignment));
    memory = nullptr;
    
    cap = 0;
    offset = 0;
    alignment = 0;
}

inline StackAllocator::StackAllocator(StackAllocator&& other) noexcept :
memory(other.memory),
cap(other.cap),
offset(other.offset),
alignment(other.alignment) {
    other.memory = nullptr;
    other.cap = 0;
    other.offset = 0;
    other.alignment = 0;
}

inline StackAllocator& StackAllocator::operator=(StackAllocator&& other) noexcept {
    if(this != &other) {
        ::operator delete(memory, std::align_val_t(alignment));
        
        memory = other.memory;
        cap = other.cap;
        offset = other.offset;
        alignment = other.alignment;
        
        other.memory = nullptr;
        other.cap = 0;
        other.offset = 0;
        other.alignment = 0;
    }
    
    return *this;
}

// Memory management 
inline void* StackAllocator::allocate(size_t size, size_t request_alignment) {
    size_t current = reinterpret_cast<size_t>(memory + offset);
    
    size_t aligned = alignForward(current, request_alignment);
    
    size_t adjustment = aligned - current;
    
    if(offset + adjustment + size > cap) {
        return nullptr;
    }
    
    byte* ptr = reinterpret_cast<byte*>(aligned);
    
    offset += adjustment + size;
    
    return ptr;
}

inline size_t StackAllocator::getMarker() const noexcept {
    return offset;
}

inline void StackAllocator::freeToMarker(size_t marker) {
    if(marker > offset) {
        return;
    }
    
    offset = marker;
}

// Object Lifecycle
template<typename T, typename... Args>
T* StackAllocator::create(Args&&... args) {
    void* raw = allocate(sizeof(T), alignof(T));
    
    if(raw == nullptr) {
        return nullptr;
    }
    
    new (raw) T(std::forward<Args>(args)...);
    
    return static_cast<T*>(raw);
}

template<typename T>
void StackAllocator::destroy(T* ptr) {
    if(!owns(ptr)) {
        return;
    }
    
    ptr->~T();
}

// Debug / Safety
inline bool StackAllocator::owns(void* ptr) const noexcept {
    byte* address = reinterpret_cast<byte*>(ptr);
    
    byte* start = memory;
    byte* end = memory + cap;
    
    return address >= start && address < end;
}

// State Management 
void StackAllocator::clear() {
    offset = 0;
}

// Capacity
size_t StackAllocator::used() const noexcept {
    return offset;
}

size_t StackAllocator::remaining() const noexcept {
    return cap - offset;
}

size_t StackAllocator::capacity() const noexcept {
    return cap;
}

