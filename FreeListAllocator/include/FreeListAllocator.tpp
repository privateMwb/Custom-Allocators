// Alignment Utilities
inline size_t FreeListAllocator::alignForward(size_t ptr, size_t alignment) {
	return (ptr + alignment - 1) & ~(alignment - 1);
}

// Free List Management
inline void FreeListAllocator::coalesce() {
    FreeBlock* current = freeList;
    
    while(current && current->next) {
        byte* currentEnd = reinterpret_cast<byte*>(current) + current->size;
        byte* nextStart = reinterpret_cast<byte*>(current->next);
        
        if(currentEnd == nextStart){
            current->size += current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

// Constructors & Destructor
inline FreeListAllocator::FreeListAllocator(size_t size, size_t alignment) :
	memory(static_cast<byte*>(:: operator new(size, std::align_val_t(alignment)))),
	cap(size),
	alignment(alignment),
	usedMemory(0) {
	freeList = reinterpret_cast<FreeBlock*>(memory);
	freeList->size = cap;
	freeList->next = nullptr;
}

inline FreeListAllocator::~FreeListAllocator() {
	::operator delete(memory, std::align_val_t(alignment));

	memory = nullptr;
	cap = 0;
	alignment = 0;
	usedMemory = 0;

	freeList = nullptr;
}

inline FreeListAllocator::FreeListAllocator(FreeListAllocator&& other) noexcept :
	memory(other.memory),
	cap(other.cap),
	alignment(other.alignment),
	usedMemory(other.usedMemory),
	freeList(other.freeList) {
	other.memory = nullptr;
	other.cap = 0;
	other.alignment = 0;
	other.usedMemory = 0;
	other.freeList = nullptr;
}

inline FreeListAllocator& FreeListAllocator::operator=(FreeListAllocator&& other) noexcept {
	if(this != &other) {
		::operator delete(memory, std::align_val_t(alignment));

		memory = other.memory;
		cap = other.cap;
		alignment = other.alignment;
		usedMemory = other.usedMemory;
		freeList = other.freeList;

		other.memory = nullptr;
		other.cap = 0;
		other.alignment = 0;
		other.usedMemory = 0;
		other.freeList = nullptr;
	}
	
	return *this;
}

// Memory Management
inline void* FreeListAllocator::allocate(size_t size, size_t request_alignment) {
    if(freeList == nullptr) {
        return nullptr;
    }
    
    FreeBlock* prev = nullptr;
    FreeBlock* current = freeList;
    
    size_t adjustment = 0;
    size_t requiredSize = 0;
    size_t alignedAddress = 0;
    
    while(current) {
        size_t currentAddress = reinterpret_cast<size_t>(current);
        
        alignedAddress = alignForward(currentAddress + sizeof(AllocationHeader), request_alignment);
        
        adjustment = alignedAddress - currentAddress;
        
        requiredSize = adjustment + size;
        
        if(current->size >= requiredSize) {
            break;
        }
        
        prev = current;
        current = current->next;
    }
    
    if(current == nullptr) {
        return nullptr;
    }
    
    size_t remainingSize = current->size - requiredSize;
    
    if(remainingSize >= sizeof(FreeBlock)){
        FreeBlock* nextBlock = reinterpret_cast<FreeBlock*>(reinterpret_cast<byte*>(current) + requiredSize);
    
        nextBlock->size = remainingSize;
        nextBlock->next = current->next;
    
        if(prev == nullptr) {
            freeList = nextBlock;
        } else {
            prev->next = nextBlock;
        }
    } else {
        requiredSize = current->size;
    
        if(prev == nullptr) {
            freeList = current->next;
        } else {
            prev->next = current->next;
        }
    }
    
    AllocationHeader* header = reinterpret_cast<AllocationHeader*>(alignedAddress - sizeof(AllocationHeader));
    
    header->size = requiredSize;
    header->adjustment = adjustment;
    
    usedMemory += requiredSize;
    
    return reinterpret_cast<void*>(alignedAddress);
}

inline void FreeListAllocator::deallocate(void* ptr) {
    if(ptr == nullptr || !owns(ptr)) {
        return;
    }
    byte* bytePtr = reinterpret_cast<byte*>(ptr);
    
    AllocationHeader* header = reinterpret_cast<AllocationHeader*>(bytePtr - sizeof(AllocationHeader));

    byte* startBlock = bytePtr - header->adjustment;
    
    FreeBlock* block = reinterpret_cast<FreeBlock*>(startBlock);
    block->size = header->size;
    
    if(freeList == nullptr) {
        freeList = block;
        block->next = nullptr;
    } else {
        FreeBlock* prev = nullptr;
        FreeBlock* current = freeList;
        
        while(current && current < block) {
            prev = current;
            current = current->next;
        }
        
        if(prev == nullptr) {
            block->next = freeList;
            freeList = block;
        } else {
            block->next = current;
            prev->next = block;
        }
        
    }
    
    usedMemory -= header->size;
    
    coalesce();
}

// Object Lifecycle
template<typename T, typename... Args>
T* FreeListAllocator::create(Args&&... args) {
    void* raw = allocate(sizeof(T), alignof(T));
    
    if(raw == nullptr) {
        return nullptr;
    }
    
    return new (raw) T(std::forward<Args>(args)...);
}

template<typename T>
void FreeListAllocator::destroy(T* ptr) {
    if(ptr == nullptr || !owns(ptr)) {
        return;
    }
    
    ptr->~T();
    deallocate(ptr);
}

// Debug / Safety
inline bool FreeListAllocator::owns(void* ptr) const noexcept {
    byte* address = reinterpret_cast<byte*>(ptr);
    byte* start = memory;
    byte* end = memory + cap;
    
    return address >= start && address < end;
}

// Capacity
inline size_t FreeListAllocator::used() const noexcept {
    return usedMemory;
}

inline size_t FreeListAllocator::remaining() const noexcept {
    return cap - usedMemory;
}

inline size_t FreeListAllocator::capacity() const noexcept {
    return cap;
}
