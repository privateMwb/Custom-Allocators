// ArenaAllocator Examples
// Demonstrates basic usage of the arena allocator:
//
// - basic allocation (typed and raw allocation)
// - object lifecycle (create/destroy)
// - arena reset and memory reuse
// - single frame management (ArenaScope)
// - nested frame management
// - pointer ownership checks
// - out-of-memory handling
// - move construction and move assignment
//
// These examples illustrate the core features and intended usage of ArenaAllocator.

#include <iostream>

#include "ArenaAllocator.h"
#include "ArenaScope.h"

// Basic Allocation
// shows typed and raw allocation using allocate<T>() and allocate(size, align)
void basicAllocation() {
    ArenaAllocator arena(256);

    int*    i = arena.allocate<int>();
    double* d = arena.allocate<double>();
    char*   c = arena.allocate<char>();

    *i = 42;
    *d = 3.14;
    *c = 'A';

    void* raw = arena.allocate(32, 16);
    (void)raw;
}

// Lifecycle
// shows object construction and destruction using create<T>() and destroy<T>()
void lifecycle() {
    struct Vec3 {
        float x, y, z;
        Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
        ~Vec3() {}
    };

    ArenaAllocator arena(256);

    Vec3* v = arena.create<Vec3>(1.0f, 2.0f, 3.0f);
    arena.destroy(v);
}

// Reset
// shows how reset() wipes the offset and allows address reuse
void reset() {
    ArenaAllocator arena(64);

    int* a  = arena.allocate<int>(); *a  = 1;
    int* b  = arena.allocate<int>(); *b  = 2;
    (void)b;

    arena.reset();

    // Same address as a is reused
    int* a2 = arena.allocate<int>(); *a2 = 99;
    (void)a2;
}

// Single Frame
// shows RAII frame management using ArenaScope
void singleFrame() {
    ArenaAllocator arena(256);

    int* base = arena.allocate<int>(); *base = 0;

    {
        ArenaScope scope(arena);

        int*    x = arena.allocate<int>();    *x = 10;
        double* y = arena.allocate<double>(); *y = 2.5;
        (void)x; (void)y;

        // x and y are reclaimed when scope exits
    }
}

// Nested Frames
// shows up to kMaxFrameDepth levels of nested ArenaScope frames
void nestedFrames() {
    ArenaAllocator arena(512);

    (void)arena.allocate<int>(); // base

    {
        ArenaScope outer(arena);
        (void)arena.allocate<double>();

        {
            ArenaScope mid(arena);
            (void)arena.allocate<float>();

            {
                ArenaScope inner(arena);
                (void)arena.allocate<char>();
                // inner reclaimed here
            }
            // mid reclaimed here
        }
        // outer reclaimed here
    }
}

// Owns
// shows pointer ownership boundary checks using owns()
void owns() {
    ArenaAllocator arena(256);

    int* inside  = arena.allocate<int>();
    int  outside = 0;

    bool a = arena.owns(inside);   // true
    bool b = arena.owns(&outside); // false
    bool c = arena.owns(nullptr);  // false

    (void)a; (void)b; (void)c;
}

// Out of Memory
// shows graceful nullptr return when the arena runs out of memory
void oom() {
    ArenaAllocator arena(8);

    int*  a   = arena.allocate<int>();  // may succeed
    int*  b   = arena.allocate<int>();  // may succeed or nullptr
    void* big = arena.allocate(16, 1); // nullptr — exceeds capacity

    (void)a; (void)b; (void)big;
}

// Move
// shows move construction and move assignment, verifying source is zeroed
void move() {
    // Move construction
    {
        ArenaAllocator arena(256);
        int* x = arena.allocate<int>(); *x = 77;

        ArenaAllocator arena2(std::move(arena));
        // arena  : used = 0, cap = 0 (zeroed)
        // arena2 : owns the buffer, *x still valid
    }

    // Move assignment
    {
        ArenaAllocator arena(256);
        int* x = arena.allocate<int>(); *x = 55;

        ArenaAllocator arena2(64);
        arena2 = std::move(arena);
        // arena  : used = 0, cap = 0 (zeroed)
        // arena2 : owns the buffer, *x still valid
    }
}

int main() {
    basicAllocation();
    lifecycle();
    reset();
    singleFrame();
    nestedFrames();
    owns();
    oom();
    move();

    return 0;
}
