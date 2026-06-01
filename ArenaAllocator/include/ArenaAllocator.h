#pragma once

#include <cstddef>
#include <new>
#include <utility>

using std::size_t;
using std::byte;

class ArenaAllocator {
public:
    // Debug Statistics
	struct Stats {
		size_t totalAllocated = 0;
		size_t currentUsed = 0;
		size_t peakUsed = 0;
		size_t allocations = 0;
	};
	
private:
	// Core Memory
	byte* memory;
	size_t cap;
	size_t offset;

	// Frame Tracking
	size_t frameStartOffset;

	// Debug Stats
	Stats stats;
	
	// Alignment Utilities
	static size_t alignForward(size_t ptr, size_t alignment);
	
public:
	// Constructors & Destructor
	explicit ArenaAllocator(size_t size);
	~ArenaAllocator();

	ArenaAllocator(const ArenaAllocator&) = delete;
	ArenaAllocator& operator=(const ArenaAllocator&) = delete;

	ArenaAllocator(ArenaAllocator&& other) noexcept;
	ArenaAllocator& operator=(ArenaAllocator&& other) noexcept;

	// Memory Management
	void* allocate(size_t size, size_t alignment);
	template<typename T>
	T* allocate();

	// Object Lifecycle
	template<typename T, typename... Args>
	T* create(Args&&... args);

	template<typename T>
	void destroy(T* ptr);

	// Frame Management
	void beginFrame() noexcept;
	void endFrame() noexcept;

	// Debug / Safety
	bool owns(void* ptr) const noexcept;

	// State Management
	void reset() noexcept;

	const Stats& getStats() const noexcept;

	// Capacity
	size_t capacity() const noexcept;
	size_t used() const noexcept;
	size_t remaining() const noexcept;
};

#include "ArenaAllocator.tpp"

