// FreeListAllocator Examples
// Demonstrates basic usage of the free-list allocator:
//
// - basic allocation (allocate/deallocate)
// - object lifecycle (create/destroy)
// - deallocation and block reuse
// - coalescing of adjacent free blocks
// - pointer ownership checks
// - out-of-memory handling
// - move construction and move assignment
//
// These examples illustrate the core features and intended usage of FreeListAllocator.

#include "FreeListAllocator.h"

// Basic Allocation
// shows allocate() and deallocate() usage
void basicAllocation() {
	FreeListAllocator allocator(256);

	void* a = allocator.allocate(sizeof(int), alignof(int));
	void* b = allocator.allocate(sizeof(int), alignof(int));
	void* c = allocator.allocate(sizeof(int), alignof(int));

	*static_cast<int*>(a) = 1;
	*static_cast<int*>(b) = 2;
	*static_cast<int*>(c) = 3;

	allocator.deallocate(a);
	allocator.deallocate(b);
	allocator.deallocate(c);
}

// Lifecycle
// shows object construction and destruction using create<T>() and destroy<T>()
void lifecycle() {
	struct Vec3 {
		float x, y, z;
		Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
		~Vec3() {}
	};

	FreeListAllocator allocator(256);

	Vec3* v = allocator.create<Vec3>(1.0f, 2.0f, 3.0f);
	allocator.destroy(v);
}

// Deallocate Reuse
// shows that a deallocated block is immediately available for reuse
void deallocateReuse() {
	FreeListAllocator allocator(256);

	void* a = allocator.allocate(sizeof(int), alignof(int));
	allocator.deallocate(a);

	// Same or equivalent block is returned
	void* b = allocator.allocate(sizeof(int), alignof(int));
	(void)b;
	allocator.deallocate(b);
}

// Coalesce
// shows adjacent free blocks merging back into one after deallocation
void coalesce() {
	FreeListAllocator allocator(256);

	void* a = allocator.allocate(sizeof(int), alignof(int));
	void* b = allocator.allocate(sizeof(int), alignof(int));
	void* c = allocator.allocate(sizeof(int), alignof(int));

	allocator.deallocate(a);
	allocator.deallocate(b);
	allocator.deallocate(c);

	// No suitable block found — coalesce triggered inside allocate(), large allocation fits
	void* large = allocator.allocate(128, alignof(std::max_align_t));
	(void)large;
	allocator.deallocate(large);
}

// Owns
// shows pointer ownership boundary checks using owns()
void owns() {
	FreeListAllocator allocator(256);

	void* inside  = allocator.allocate(sizeof(int), alignof(int));
	int   outside = 0;

	bool a = allocator.owns(inside);   // true
	bool b = allocator.owns(&outside); // false
	bool c = allocator.owns(nullptr);  // false

	(void)a;
	(void)b;
	(void)c;

	allocator.deallocate(inside);
}

// Out of Memory
// shows graceful nullptr return when the allocator is exhausted
void oom() {
	static constexpr std::size_t BLOCK_SIZE = sizeof(int) + sizeof(std::size_t) * 2;

	FreeListAllocator allocator(BLOCK_SIZE);

	void* a = allocator.allocate(sizeof(int), alignof(int)); // ok
	void* b = allocator.allocate(sizeof(int), alignof(int)); // nullptr — exhausted

	(void)b;

	// Deallocate and try again
	allocator.deallocate(a);
	void* c = allocator.allocate(sizeof(int), alignof(int)); // ok — block reused
	(void)c;

	allocator.deallocate(c);
}

// Move
// shows move construction and move assignment, verifying source is zeroed
void move() {
	// Move construction
	{
		FreeListAllocator allocator(256);
		void* p = allocator.allocate(sizeof(int), alignof(int));
		(void)p;

		FreeListAllocator allocator2(std::move(allocator));
		// allocator  : capacity = 0, used = 0 (zeroed)
		// allocator2 : owns the buffer, p still valid
	}

	// Move assignment
	{
		FreeListAllocator allocator(256);
		void* p = allocator.allocate(sizeof(int), alignof(int));
		(void)p;

		FreeListAllocator allocator2(128);
		allocator2 = std::move(allocator);
		// allocator  : capacity = 0, used = 0 (zeroed)
		// allocator2 : owns the buffer, p still valid
	}
}

// Entry Point
int main() {
	basicAllocation();
	lifecycle();
	deallocateReuse();
	coalesce();
	owns();
	oom();
	move();

	return 0;
}