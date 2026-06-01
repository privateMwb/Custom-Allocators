// Alignment Utilities 
inline size_t PoolAllocator::alignForward(size_t ptr, size_t alignment) {
    return (ptr + alignment - 1) & ~(alignment - 1);
}

// Constructors & Destructor
inline PoolAllocator::PoolAllocator(size_t blockSize, size_t blockCount, size_t alignment) :
	blockSize(blockSize < sizeof(FreeNode) ? sizeof(FreeNode) : blockSize),
	blockCount(blockCount),
	alignment(alignment),
	freeBlockCount(blockCount) {
	stride = alignForward(blockSize, alignment);
	memory = static_cast<byte*>(::operator new(stride * blockCount, std::align_val_t(alignment)));
	freeList = reinterpret_cast<FreeNode*>(memory);
	
	FreeNode* current = freeList;

	for(size_t i = 0; i<blockCount-1; ++i) {
		byte* nextBlock = reinterpret_cast<byte*>(current) + stride;

		current->next = reinterpret_cast<FreeNode*>(nextBlock);

		current = current->next;
	}

	current->next = nullptr;
}

inline PoolAllocator::~PoolAllocator() {
	::operator delete(memory, std::align_val_t(alignment));

	memory = nullptr;

	blockSize = 0;
	stride = 0;
	blockCount = 0;
	alignment = 0;
	
	freeBlockCount = 0;
	
	freeList = nullptr;
}

inline PoolAllocator::PoolAllocator(PoolAllocator&& other) noexcept :
	memory(other.memory),
	blockSize(other.blockSize),
	stride(other.stride),
	blockCount(other.blockCount),
	alignment(other.alignment),
	freeBlockCount(other.freeBlockCount),
	freeList(other.freeList) {
	other.memory = nullptr;
	other.blockSize= 0;
	other.stride = 0;
	other.blockCount = 0;
	other.alignment = 0;
	other.freeBlockCount = 0;
	other.freeList = nullptr;
}

inline PoolAllocator& PoolAllocator::operator=(PoolAllocator&& other) noexcept {
	if(this != &other) {
		::operator delete(memory, std::align_val_t(alignment));

		memory = other.memory;
		blockSize = other.blockSize;
		stride = other.stride;
		blockCount = other.blockCount;
		alignment = other.alignment;
		freeBlockCount = other.freeBlockCount;
		freeList = other.freeList;

		other.memory = nullptr;
		other.blockSize = 0;
		other.stride = 0;
		other.blockCount = 0;
		other.alignment = 0;
		other.freeBlockCount = 0;
		other.freeList = nullptr;
	}
	
	return *this;
}

// Memory Management 
inline void* PoolAllocator::allocate() {
    if(freeList == nullptr) {
        return nullptr;
    }
    
    FreeNode* node = freeList;
    
    freeList = freeList->next;
    
    --freeBlockCount;
    
    return reinterpret_cast<void*>(node);
}

inline void PoolAllocator::deallocate(void* ptr) {
    if(ptr == nullptr) {
        return;
    }
    
    FreeNode* node = reinterpret_cast<FreeNode*>(ptr);
    
    node->next = freeList;
    
    freeList = node;
    
    ++freeBlockCount;
}

// Object Lifecycle
template<typename T, typename... Args>
T* PoolAllocator::create(Args&&... args) {
    if(sizeof(T) > blockSize || alignof(T) > alignment) {
        return nullptr;
    }
    
    void* raw = allocate();
    
    if(raw == nullptr) {
        return nullptr;
    } 
    
    T* ptr = static_cast<T*>(raw);
    
    new (ptr) T(std::forward<Args>(args)...);
    
    return ptr;
}

template<typename T>
void PoolAllocator::destroy(T* ptr) {
    if(!owns(ptr)) return;
    
    ptr->~T();
    
    deallocate(ptr);
}

// Debug / Safety
inline bool PoolAllocator::owns(void* ptr) const noexcept {
    byte* start = memory;
    byte* end = memory + (stride * blockCount);
    
    byte* address = reinterpret_cast<byte*>(ptr);
    
    return address >= start && address < end;
}

// Capacity
size_t PoolAllocator::capacity() const noexcept {
    return stride * blockCount;
}

size_t PoolAllocator::usedBlocks() const noexcept {
    return blockCount - freeBlockCount;
}

size_t PoolAllocator::freeBlocks() const noexcept {
    return freeBlockCount;
}

size_t PoolAllocator::totalBlocks() const noexcept {
    return blockCount;
}

size_t PoolAllocator::block_size() const noexcept {
    return blockSize;
}



