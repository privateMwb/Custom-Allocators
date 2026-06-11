// PoolAllocator Unit Test Suite
// Tests correctness of fixed-size pool allocation and memory management:
//
// - basic allocation
// - exact pool utilization
// - out-of-memory handling
// - object lifecycle (create/destroy)
// - type size validation
// - deallocation and block reuse
// - ownership detection
// - allocation statistics
// - move semantics
//
// These tests validate correctness of PoolAllocator under normal
// allocation patterns and edge cases.

#include <iostream>
#include <cassert>

#include "PoolAllocator.h"

// Basic Allocation Test
// verifies successful allocation and deallocation of a single block
void basic() {
    PoolAllocator pool(sizeof(int), 8);

    void* p = pool.allocate();
    assert(p != nullptr);

    int* x = static_cast<int*>(p);
    *x = 42;
    assert(*x == 42);

    assert(pool.usedBlocks()  == 1);
    assert(pool.freeBlocks()  == 7);
    assert(pool.totalBlocks() == 8);

    pool.deallocate(p);

    assert(pool.usedBlocks() == 0);
    assert(pool.freeBlocks() == 8);

    std::cout << "\n[PASS] Basic Test\n";
}

// Exact Fill Test
// verifies all blocks can be allocated until the pool is exhausted
void exactFill() {
    PoolAllocator pool(sizeof(int), 4);

    void* a = pool.allocate();
    void* b = pool.allocate();
    void* c = pool.allocate();
    void* d = pool.allocate();

    assert(a != nullptr);
    assert(b != nullptr);
    assert(c != nullptr);
    assert(d != nullptr);

    assert(pool.usedBlocks() == 4);
    assert(pool.freeBlocks() == 0);

    pool.deallocate(a);
    pool.deallocate(b);
    pool.deallocate(c);
    pool.deallocate(d);

    assert(pool.usedBlocks() == 0);
    assert(pool.freeBlocks() == 4);

    std::cout << "\n[PASS] Exact Fill Test\n";
}

// Out of Memory Test
// verifies allocation returns nullptr when no free blocks remain
void oom() {
    PoolAllocator pool(sizeof(int), 2);

    void* a = pool.allocate();
    void* b = pool.allocate();

    assert(a != nullptr);
    assert(b != nullptr);

    void* c = pool.allocate();
    assert(c == nullptr);

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

    PoolAllocator pool(sizeof(Foo), 4);

    bool destroyed = false;

    Foo* f = pool.create<Foo>(99, destroyed);
    assert(f != nullptr);
    assert(f->value == 99);
    assert(!destroyed);

    pool.destroy(f);
    assert(destroyed);

    assert(pool.usedBlocks() == 0);

    std::cout << "\n[PASS] Lifecycle Test\n";
}

// Type Mismatch Test
// verifies oversized objects cannot be allocated from the pool
void typeMismatch() {
    struct Big { char data[256]; };

    // blockSize is smaller than Big
    PoolAllocator pool(sizeof(int), 4);

    Big* p = pool.create<Big>();
    assert(p == nullptr);

    // pool should be untouched
    assert(pool.usedBlocks() == 0);
    assert(pool.freeBlocks() == 4);

    std::cout << "\n[PASS] Type Mismatch Test\n";
}

// Deallocate Reuse Test
// verifies freed blocks are immediately available for reuse
void deallocateReuse() {
    PoolAllocator pool(sizeof(int), 4);

    void* a = pool.allocate();
    assert(a != nullptr);

    pool.deallocate(a);

    // Pool is LIFO — next allocation should return the same address
    void* b = pool.allocate();
    assert(b == a);

    pool.deallocate(b);

    std::cout << "\n[PASS] Deallocate Reuse Test\n";
}

// Owns Test
// verifies pointer ownership detection inside the pool
void owns() {
    PoolAllocator pool(sizeof(int), 4);

    void* inside  = pool.allocate();
    int   outside = 0;

    assert( pool.owns(inside));
    assert(!pool.owns(&outside));
    assert(!pool.owns(nullptr));

    pool.deallocate(inside);

    std::cout << "\n[PASS] Owns Test\n";
}

// Statistics Test
// verifies allocation statistics are tracked correctly
void stats() {
    PoolAllocator pool(sizeof(int), 4);

    void* a = pool.allocate();
    void* b = pool.allocate();

    const auto& s = pool.getStats();

    assert(s.allocations   == 2);
    assert(s.deallocations == 0);
    assert(s.totalAllocated == pool.blockStride() * 2);
    assert(s.peakUsed      == 2);

    pool.deallocate(a);

    assert(s.deallocations == 1);
    assert(s.peakUsed      == 2); // peak preserved after deallocation

    pool.deallocate(b);

    assert(s.deallocations == 2);
    assert(s.peakUsed      == 2);

    std::cout << "\n[PASS] Stats Test\n";
}

// Move Semantics Test
// verifies move construction and move assignment transfer ownership
void move() {
    // Move construction
    {
        PoolAllocator pool(sizeof(int), 4);

        void* p = pool.allocate();
        assert(p != nullptr);

        PoolAllocator pool2(std::move(pool));

        assert(pool2.usedBlocks()  == 1);
        assert(pool2.totalBlocks() == 4);
        assert(pool2.capacity()    == pool2.blockStride() * 4);

        assert(pool.usedBlocks()   == 0);
        assert(pool.totalBlocks()  == 0);
        assert(pool.capacity()     == 0);
    }

    // Move assignment
    {
        PoolAllocator pool(sizeof(int), 4);

        void* p = pool.allocate();
        assert(p != nullptr);

        PoolAllocator pool2(sizeof(int), 2);
        pool2 = std::move(pool);

        assert(pool2.usedBlocks()  == 1);
        assert(pool2.totalBlocks() == 4);

        assert(pool.usedBlocks()   == 0);
        assert(pool.totalBlocks()  == 0);
        assert(pool.capacity()     == 0);
    }

    std::cout << "\n[PASS] Move Test\n";
}

// Entry Point
int main() {
    basic();
    exactFill();
    oom();
    lifecycle();
    typeMismatch();
    deallocateReuse();
    owns();
    stats();
    move();

    return 0;
}


