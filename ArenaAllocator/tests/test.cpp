// ArenaAllocator Unit Test Suite
// Tests correctness of arena allocation and memory management:
//
// - basic allocation
// - exact capacity allocation
// - out-of-memory handling
// - reset and memory reuse
// - alignment guarantees
// - object lifecycle (create/destroy)
// - scoped frame allocation
// - nested frame restoration
// - ownership detection
// - allocation statistics
// - move semantics
// - zero-capacity arena
//
// These tests validate correctness of ArenaAllocator under common
// allocation patterns and edge cases.

#include <iostream>
#include <cassert>
#include <cstddef>

#include "ArenaAllocator.h"
#include "ArenaScope.h"

// Basic Allocation Test
// verifies successful allocation of a single object
void basic() {
	ArenaAllocator arena(1024);

	int* x = arena.allocate<int>();

	assert(x != nullptr);
	assert(*x = 42);
	assert(arena.used() == sizeof(int));

	std::cout << "\n[PASS] Basic Test\n";
}

// Exact Fill Test
// verifies allocation succeeds until arena capacity is exhausted
void exactFill() {
	ArenaAllocator arena(sizeof(int));

	int* x = arena.allocate<int>();
	int* y = arena.allocate<int>();

	assert(x != nullptr);
	assert(y == nullptr);

	std::cout << "\n[PASS] Exact Fill Test\n";
}

// Out of Memory Test
// verifies allocation returns nullptr when capacity is exceeded
void oom() {
	ArenaAllocator arena(8);

	void* p = arena.allocate(16, 1);

	assert(p == nullptr);

	std::cout << "\n[PASS] Out of Memory Test\n";
}

// Reset Reuse Test
// verifies reset() reclaims all arena memory for reuse
void resetReuse() {
	ArenaAllocator arena(sizeof(int));

	int* x = arena.allocate<int>();

	arena.reset();

	int*y = arena.allocate<int>();

	assert(x != nullptr);
	assert(y != nullptr);
	assert(x == y);

	std::cout << "\n[PASS] Reset Reuse Test\n";
}

// Alignment Test
// verifies allocations satisfy requested alignment requirements
void alignment() {
	ArenaAllocator arena(1024);

	char* c = arena.allocate<char>();
	assert(c != nullptr);

	double* d = arena.allocate<double>();
	assert(d != nullptr);
	assert(reinterpret_cast<std::size_t>(d) % alignof(double) == 0);

	assert(arena.used() > sizeof(char) + sizeof(double));

	int* x = arena.allocate<int>();
	assert(x != nullptr);
	assert(reinterpret_cast<std::size_t>(x) % alignof(int) == 0);

	struct alignas(16) Vec4 {
	    float x, y, z, w;
	};
	Vec4* v = arena.allocate<Vec4>();
	assert(v != nullptr);
	assert(reinterpret_cast<std::size_t>(v) % alignof(Vec4) == 0);

	std::cout << "\n[PASS] Alignment Test\n";
}

// Object Lifecycle Test
// verifies create() constructs and destroy() calls destructors
void lifecycle() {
	struct Foo {
		int value;
		bool& destroyed;

		Foo(int v, bool& d)
			: value(v)
			, destroyed(d)
		{}

		~Foo() {
			destroyed = true;
		}
	};

	ArenaAllocator arena(1024);
	bool destroyed = false;

	Foo* f = arena.create<Foo>(99, destroyed);
	assert(f != nullptr);
	assert(f->value == 99);
	assert(!destroyed);

	arena.destroy(f);
	assert(destroyed);

	std::cout << "\n[PASS] Lifecycle Test\n";
}

// ArenaScope Test
// verifies ArenaScope automatically restores arena state
void scope() {
	ArenaAllocator arena(1024);

	{
		ArenaScope scope(arena);

		int* x = arena.allocate<int>();
		double* y = arena.allocate<double>();

		assert(x != nullptr);
		assert(y != nullptr);

		assert(arena.used() > 0);
	}

	assert(arena.used() == 0);

	std::cout << "\n[PASS] Scope Test\n";
}

// Nested Frames Test
// verifies nested frames restore allocation offsets correctly
void nestedFrames() {
	ArenaAllocator arena(1024);

	(void)arena.allocate<int>();
	size_t baseUsed = arena.used();

	{
		ArenaScope outer(arena);
		(void)arena.allocate<int>();
		size_t outerUsed = arena.used();

		{
			ArenaScope inner(arena);
			(void)arena.allocate<int>();
			assert(arena.used() > outerUsed);
		}

		assert(arena.used() == outerUsed);
	}

	assert(arena.used() == baseUsed);

	std::cout << "\n[PASS] Nested Frames Test\n";
}

// Owns Test
// verifies pointer ownership detection inside the arena
void owns() {
	ArenaAllocator arena(1024);

	int* inside = arena.allocate<int>();
	int outside = 0;

	assert(arena.owns(inside));
	assert(!arena.owns(&outside));
	assert(!arena.owns(nullptr));

	std::cout << "\n[PASS] Owns Test\n";
}

// Statistics Test
// verifies allocation statistics are tracked correctly
void stats() {
	ArenaAllocator arena(1024);

	(void)arena.allocate<int>();
	(void)arena.allocate<double>();

	const auto& s = arena.getStats();
	assert(s.allocations    == 2);
	assert(s.totalAllocated == sizeof(int) + sizeof(double));
	assert(s.currentUsed    == arena.used());
	assert(s.peakUsed       >= s.currentUsed);

	size_t peakBefore = s.peakUsed;
	arena.reset();

	assert(s.allocations    == 0);
	assert(s.currentUsed    == 0);
	assert(s.peakUsed       == peakBefore);

	std::cout << "\n[PASS] Stats Test\n";
}

// Move Semantics Test
// verifies move construction and move assignment transfer ownership
void move() {
	{
		ArenaAllocator arena(1024);
		int* x = arena.allocate<int>();
		*x = 9;

		ArenaAllocator arena2(std::move(arena));
		
		assert(arena2.used()     == sizeof(int));
		assert(arena2.capacity() == 1024);
		assert(arena.used()      == 0);
		assert(arena.capacity()  == 0);
		assert(*x == 9);
	}
	{
		ArenaAllocator arena(1024);
		int* x = arena.allocate<int>();
		*x = 7;

		ArenaAllocator arena2(64);
		arena2 = std::move(arena);

		assert(arena2.used()     == sizeof(int));
		assert(arena2.capacity() == 1024);
		assert(arena.used()      == 0);
		assert(arena.capacity()  == 0);
		assert(*x == 7);
	}
	
	std::cout << "\n[PASS] Move Test\n";
}

// Zero Capacity Test
// verifies zero-sized arenas reject all allocations
void zeroArena() {
    ArenaAllocator arena(0);
    
    void* p = arena.allocate(1, 1);
    
    assert(p == nullptr);
    
    std::cout << "\n[PASS] Zero Arena Test\n";
}

// Entry Point
int main() {
	basic();
	exactFill();
	oom();
	resetReuse();
	alignment();
	lifecycle();
	scope();
	nestedFrames();
	owns();
	stats();
	move();
	zeroArena();
	
	return 0;
}


