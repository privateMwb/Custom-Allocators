// PoolAllocator Examples
// Demonstrates basic usage of the fixed-size pool allocator:
//
// - basic allocation (allocate/deallocate)
// - object lifecycle (create/destroy)
// - block reuse after deallocation
// - pointer ownership checks
// - out-of-memory handling
// - move construction and move assignment
//
// These examples illustrate the core features and intended usage of PoolAllocator.

#include <iostream>

#include "PoolAllocator.h"

// Basic Allocation
// shows allocate() and deallocate() usage
void basicAllocation() {
    PoolAllocator pool(sizeof(int), 8);

    void* a = pool.allocate();
    void* b = pool.allocate();
    void* c = pool.allocate();

    *static_cast<int*>(a) = 1;
    *static_cast<int*>(b) = 2;
    *static_cast<int*>(c) = 3;

    pool.deallocate(a);
    pool.deallocate(b);
    pool.deallocate(c);
}

// Lifecycle
// shows object construction and destruction using create<T>() and destroy<T>()
void lifecycle() {
    struct Vec3 {
        float x, y, z;
        Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
        ~Vec3() {}
    };

    PoolAllocator pool(sizeof(Vec3), 4);

    Vec3* v = pool.create<Vec3>(1.0f, 2.0f, 3.0f);
    pool.destroy(v);
}

// Deallocate Reuse
// shows that a deallocated block is immediately available for reuse
void deallocateReuse() {
    PoolAllocator pool(sizeof(int), 4);

    void* a = pool.allocate();
    pool.deallocate(a);

    // Pool is LIFO — same block is returned
    void* b = pool.allocate();
    (void)b; // b == a
    pool.deallocate(b);
}

// Owns
// shows pointer ownership boundary checks using owns()
void owns() {
    PoolAllocator pool(sizeof(int), 4);

    void* inside  = pool.allocate();
    int   outside = 0;

    bool a = pool.owns(inside);   // true
    bool b = pool.owns(&outside); // false
    bool c = pool.owns(nullptr);  // false

    (void)a; (void)b; (void)c;

    pool.deallocate(inside);
}

// Out of Memory
// shows graceful nullptr return when the pool is exhausted
void oom() {
    PoolAllocator pool(sizeof(int), 2);

    void* a = pool.allocate(); // ok
    void* b = pool.allocate(); // ok
    void* c = pool.allocate(); // nullptr — pool exhausted

    (void)c;

    // Free one block and allocate again
    pool.deallocate(a);
    void* d = pool.allocate(); // ok — block reused
    (void)d;

    pool.deallocate(b);
    pool.deallocate(d);
}

// Move
// shows move construction and move assignment, verifying source is zeroed
void move() {
    // Move construction
    {
        PoolAllocator pool(sizeof(int), 4);
        void* p = pool.allocate();
        (void)p;

        PoolAllocator pool2(std::move(pool));
        // pool  : totalBlocks = 0, capacity = 0 (zeroed)
        // pool2 : owns the buffer, p still valid
    }

    // Move assignment
    {
        PoolAllocator pool(sizeof(int), 4);
        void* p = pool.allocate();
        (void)p;

        PoolAllocator pool2(sizeof(int), 2);
        pool2 = std::move(pool);
        // pool  : totalBlocks = 0, capacity = 0 (zeroed)
        // pool2 : owns the buffer, p still valid
    }
}

// Entry Point
int main() {
    basicAllocation();
    lifecycle();
    deallocateReuse();
    owns();
    oom();
    move();

    return 0;
}
