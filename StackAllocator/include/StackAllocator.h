#pragma once

#include <cstddef>
#include <new>
#include <utility>

// Forward declaration — see StackScope.h for RAII marker management
class StackScope;

class StackAllocator {
public:
	// Debug Statistics
	struct Stats {
		std::size_t totalAllocated = 0;
		std::size_t currentUsed    = 0;
		std::size_t peakUsed       = 0;
		std::size_t allocations    = 0;
	};

private:
	// Core Memory
	std::byte* memory;

	// Stack Configuration
	std::size_t cap;
	std::size_t offset;
	std::size_t alignment;

	// Debug Stats
	Stats stats;

	// Alignment Utilities
	[[nodiscard]] static constexpr std::size_t alignForward(std::size_t ptr, std::size_t alignment) noexcept;
	[[nodiscard]] static constexpr bool isPowerOfTwo(std::size_t value) noexcept;

public:
	// Constructors & Destructor
	explicit StackAllocator(std::size_t size, std::size_t alignment = alignof(std::max_align_t));
	~StackAllocator();

	StackAllocator(const StackAllocator&)            = delete;
	StackAllocator& operator=(const StackAllocator&) = delete;

	StackAllocator(StackAllocator&& other) noexcept;
	StackAllocator& operator=(StackAllocator&& other) noexcept;

	// Memory Management
	[[nodiscard]] void* allocate(std::size_t size, std::size_t request_alignment = alignof(std::max_align_t)) noexcept;

	[[nodiscard]] std::size_t getMarker() const noexcept;

	// marker must be <= current offset (assert-guarded)
	void freeToMarker(std::size_t marker) noexcept;

	// Object Lifecycle
	template<typename T, typename... Args>
	[[nodiscard]] T* create(Args&&... args);

	template<typename T>
	void destroy(T* ptr) noexcept;

	// State Management
	void reset() noexcept;

	// Introspection
	[[nodiscard]] bool owns(const void* ptr) const noexcept;

	[[nodiscard]] const Stats& getStats() const noexcept;

	[[nodiscard]] std::size_t used()      const noexcept;
	[[nodiscard]] std::size_t remaining() const noexcept;
	[[nodiscard]] std::size_t capacity()  const noexcept;
};

#include "StackAllocator.tpp"