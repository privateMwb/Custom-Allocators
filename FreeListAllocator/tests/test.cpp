

// FreeListAllocator Unit Test Suite
// Tests correctness of free-list allocation and memory management:
//
// - basic allocation
// - exact allocator utilization
// - out-of-memory handling
// - object lifecycle (create/destroy)
// - deallocation and block reuse
// - coalescing of adjacent free blocks
// - fragmentation handling
// - ownership detection
// - allocation statistics
// - move semantics
//
// These tests validate correctness of FreeListAllocator under normal
// allocation patterns and edge cases.

#include <iostream>
#include <cassert>

#include "FreeListAllocator.h"

// Basic Allocation Test
// verifies successful allocation and deallocation of a single block
void basic() {
    FreeListAllocator allocator(256);

    void* p = allocator.allocate(sizeof(int), alignof(int));
    assert(p != nullptr);

    int* x = static_cast<int*>(p);
    *x = 42;
    assert(*x == 42);

    assert(allocator.used()      > 0);
    assert(allocator.remaining() < 256);
    assert(allocator.capacity()  == 256);

    allocator.deallocate(p);

    assert(allocator.used() == 0);

    std::cout << "\n[PASS] Basic Test\n";
}

// Exact Fill Test
// verifies the allocator can be filled to capacity
void exactFill() {
    static constexpr std::size_t BLOCK = sizeof(int);
    static constexpr std::size_t COUNT = 4;

    FreeListAllocator allocator((BLOCK + sizeof(std::size_t) * 2) * COUNT);

    void* a = allocator.allocate(BLOCK, alignof(int));
    void* b = allocator.allocate(BLOCK, alignof(int));
    void* c = allocator.allocate(BLOCK, alignof(int));
    void* d = allocator.allocate(BLOCK, alignof(int));

    assert(a != nullptr);
    assert(b != nullptr);
    assert(c != nullptr);
    assert(d != nullptr);

    allocator.deallocate(a);
    allocator.deallocate(b);
    allocator.deallocate(c);
    allocator.deallocate(d);

    assert(allocator.used() == 0);

    std::cout << "\n[PASS] Exact Fill Test\n";
}

// Out of Memory Test
// verifies allocation returns nullptr when the allocator is exhausted
void oom() {
    FreeListAllocator allocator(sizeof(int) + sizeof(std::size_t) * 2);

    void* a = allocator.allocate(sizeof(int), alignof(int));
    assert(a != nullptr);

    void* b = allocator.allocate(sizeof(int), alignof(int));
    assert(b == nullptr);

    allocator.deallocate(a);

    std::cout << "\n[PASS] Out of Memory Test\n";
}

// Object Lifecycle Test
// verifies create() constructs and destroy() calls destructors
void lifecycle() {
    struct Foo {
        int value;
        bool& destroyed;

        Foo(int v, bool& d) : value(v), destroyed(d) {}
        ~Foo() { destroyed = true; }
    };

    FreeListAllocator allocator(256);

    bool destroyed = false;

    Foo* f = allocator.create<Foo>(99, destroyed);
    assert(f != nullptr);
    assert(f->value == 99);
    assert(!destroyed);

    allocator.destroy(f);
    assert(destroyed);

    assert(allocator.used() == 0);

    std::cout << "\n[PASS] Lifecycle Test\n";
}

// Deallocate Reuse Test
// verifies freed blocks are available for reuse
void deallocateReuse() {
    FreeListAllocator allocator(256);

    void* a = allocator.allocate(sizeof(int), alignof(int));
    assert(a != nullptr);

    std::size_t usedAfterAlloc = allocator.used();
    allocator.deallocate(a);

    assert(allocator.used() == 0);

    void* b = allocator.allocate(sizeof(int), alignof(int));
    assert(b != nullptr);
    assert(allocator.used() == usedAfterAlloc);

    allocator.deallocate(b);

    std::cout << "\n[PASS] Deallocate Reuse Test\n";
}

// Coalesce Test
// verifies adjacent free blocks are merged back into one
void coalesce() {
    FreeListAllocator allocator(256);

    void* a = allocator.allocate(sizeof(int), alignof(int));
    void* b = allocator.allocate(sizeof(int), alignof(int));
    void* c = allocator.allocate(sizeof(int), alignof(int));

    assert(a != nullptr);
    assert(b != nullptr);
    assert(c != nullptr);

    allocator.deallocate(a);
    allocator.deallocate(b);
    allocator.deallocate(c);

    assert(allocator.used()      == 0);
    assert(allocator.remaining() == allocator.capacity());

    // After full coalesce, a large allocation should succeed
    void* large = allocator.allocate(128, alignof(std::max_align_t));
    assert(large != nullptr);

    allocator.deallocate(large);

    std::cout << "\n[PASS] Coalesce Test\n";
}

// Fragmentation Test
// verifies non-adjacent free blocks remain usable after alternating deallocations
void fragmentation() {
    FreeListAllocator allocator(512);

    void* a = allocator.allocate(sizeof(int), alignof(int));
    void* b = allocator.allocate(sizeof(int), alignof(int));
    void* c = allocator.allocate(sizeof(int), alignof(int));
    void* d = allocator.allocate(sizeof(int), alignof(int));

    assert(a != nullptr);
    assert(b != nullptr);
    assert(c != nullptr);
    assert(d != nullptr);

    // Free alternating blocks — leaves b and d allocated, a and c free but non-adjacent
    allocator.deallocate(a);
    allocator.deallocate(c);

    // b and d still live
    *static_cast<int*>(b) = 1;
    *static_cast<int*>(d) = 2;

    assert(*static_cast<int*>(b) == 1);
    assert(*static_cast<int*>(d) == 2);

    // Free slots should still be allocatable
    void* e = allocator.allocate(sizeof(int), alignof(int));
    assert(e != nullptr);

    allocator.deallocate(b);
    allocator.deallocate(d);
    allocator.deallocate(e);

    assert(allocator.used() == 0);

    std::cout << "\n[PASS] Fragmentation Test\n";
}

// Owns Test
// verifies pointer ownership detection inside the allocator
void owns() {
    FreeListAllocator allocator(256);

    void* inside  = allocator.allocate(sizeof(int), alignof(int));
    int   outside = 0;

    assert( allocator.owns(inside));
    assert(!allocator.owns(&outside));
    assert(!allocator.owns(nullptr));

    allocator.deallocate(inside);

    std::cout << "\n[PASS] Owns Test\n";
}

// Statistics Test
// verifies allocation statistics are tracked correctly
void stats() {
    FreeListAllocator allocator(256);

    void* a = allocator.allocate(sizeof(int), alignof(int));
    void* b = allocator.allocate(sizeof(int), alignof(int));

    const auto& s = allocator.getStats();

    assert(s.allocations    == 2);
    assert(s.deallocations  == 0);
    assert(s.totalAllocated >= sizeof(int) * 2);
    assert(s.currentUsed    == allocator.used());
    assert(s.peakUsed       == allocator.used());

    allocator.deallocate(a);

    assert(s.deallocations == 1);
    assert(s.currentUsed   == allocator.used());
    assert(s.peakUsed      >= sizeof(int) * 2);

    allocator.deallocate(b);

    assert(s.deallocations == 2);
    assert(s.currentUsed   == 0);
    assert(s.peakUsed      >= sizeof(int) * 2);

    std::cout << "\n[PASS] Stats Test\n";
}

// Move Semantics Test
// verifies move construction and move assignment transfer ownership
void move() {
    // Move construction
    {
        FreeListAllocator allocator(256);

        void* p = allocator.allocate(sizeof(int), alignof(int));
        assert(p != nullptr);

        std::size_t usedBefore = allocator.used();

        FreeListAllocator allocator2(std::move(allocator));

        assert(allocator2.used()     == usedBefore);
        assert(allocator2.capacity() == 256);

        assert(allocator.used()      == 0);
        assert(allocator.capacity()  == 0);
        assert(allocator.remaining() == 0);
    }

    // Move assignment
    {
        FreeListAllocator allocator(256);

        void* p = allocator.allocate(sizeof(int), alignof(int));
        assert(p != nullptr);

        std::size_t usedBefore = allocator.used();

        FreeListAllocator allocator2(128);
        allocator2 = std::move(allocator);

        assert(allocator2.used()     == usedBefore);
        assert(allocator2.capacity() == 256);

        assert(allocator.used()      == 0);
        assert(allocator.capacity()  == 0);
        assert(allocator.remaining() == 0);
    }

    std::cout << "\n[PASS] Move Test\n";
}

// Entry Point
int main() {
    basic();
    exactFill();
    oom();
    lifecycle();
    deallocateReuse();
    coalesce();
    fragmentation();
    owns();
    stats();
    move();

    return 0;
}

