// Alignment Utilities
inline size_t ArenaAllocator::alignForward(size_t ptr, size_t alignment) {
	return (ptr + alignment - 1) & ~(alignment - 1);
}
// Constructors & Destructor
inline ArenaAllocator::ArenaAllocator(size_t size) :
	memory(static_cast<byte*>(::operator new(size))),
	cap(size),
	offset(0) {}

inline ArenaAllocator::~ArenaAllocator() {
	::operator delete(memory);

	memory = nullptr;
	cap = 0;
	offset = 0;
}

inline ArenaAllocator::ArenaAllocator(ArenaAllocator&& other) noexcept :
	memory(other.memory),
	cap(other.cap),
	offset(other.offset),
	frameStartOffset(other.frameStartOffset),
	stats(other.stats) {

	other.memory = nullptr;
	other.cap = 0;
	other.offset = 0;
	other.frameStartOffset = 0;
	other.stats = {};
}

inline ArenaAllocator&
ArenaAllocator::operator=(ArenaAllocator&& other) noexcept {

	if(this != &other) {

		::operator delete(memory);

		memory = other.memory;
		cap = other.cap;
		offset = other.offset;
		frameStartOffset = other.frameStartOffset;
		stats = other.stats;

		other.memory = nullptr;
		other.cap = 0;
		other.offset = 0;
		other.frameStartOffset = 0;
		other.stats = {};
	}

	return *this;
}

// Memory Management
inline void*
ArenaAllocator::allocate(size_t size, size_t alignment) {

	stats.allocations++;
	stats.totalAllocated += size;

	size_t current =
	    reinterpret_cast<size_t>(memory + offset);

	size_t aligned =
	    alignForward(current, alignment);

	size_t padding = aligned - current;

	if(offset + padding + size > cap) {
		return nullptr;
	}

	void* ptr = reinterpret_cast<void*>(aligned);

	offset += (padding + size);

	stats.currentUsed = offset;

	if (stats.currentUsed > stats.peakUsed) {
		stats.peakUsed = stats.currentUsed;
	}

	return ptr;
}

template<typename T>
T* ArenaAllocator::allocate() {
	return static_cast<T*>(
	           allocate(sizeof(T), alignof(T))
	       );
}

// Object Lifecycle
template<typename T, typename... Args>
T* ArenaAllocator::create(Args&&... args) {

	void* raw =
	    allocate(sizeof(T), alignof(T));

	T* ptr = static_cast<T*>(raw);

	new (ptr) T(std::forward<Args>(args)...);

	return ptr;
}

template<typename T>
void ArenaAllocator::destroy(T* ptr) {

	if(!owns(ptr)) return;

	ptr->~T();
}

// Frame Management
inline void ArenaAllocator::beginFrame() noexcept {
	frameStartOffset = offset;
}

inline void ArenaAllocator::endFrame() noexcept {
	offset = frameStartOffset;
}

// Debug / Safety
inline bool
ArenaAllocator::owns(void* ptr) const noexcept {

	auto start = memory;
	auto end = memory + cap;

	return ptr >= start && ptr < end;
}

inline const ArenaAllocator::Stats&
ArenaAllocator::getStats() const noexcept {

	return stats;
}

// State Management
inline void ArenaAllocator::reset() noexcept {
	offset = 0;
}

// Capacity
inline size_t
ArenaAllocator::capacity() const noexcept {

	return cap;
}

inline size_t
ArenaAllocator::used() const noexcept {

	return offset;
}

inline size_t
ArenaAllocator::remaining() const noexcept {

	return cap - offset;
}