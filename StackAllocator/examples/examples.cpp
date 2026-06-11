// StackAllocator Examples
// Demonstrates basic usage of the stack-based linear allocator:
//
// - basic allocation (allocate/reset)
// - object lifecycle (create/destroy)
// - marker save and rewind
// - scope-based RAII rewind
// - reset (wipe and reuse)
// - pointer ownership checks
// - out-of-memory handling
// - move construction and move assignment
//
// These examples illustrate the core features and intended usage of StackAllocator.

#include "StackAllocator.h"
#include "StackScope.h"

// Basic Allocation
// shows allocate() usage
void basicAllocation() {
    StackAllocator stack(256);

    void* a = stack.allocate(sizeof(int), alignof(int));
    void* b = stack.allocate(sizeof(int), alignof(int));
    void* c = stack.allocate(sizeof(int), alignof(int));

    *static_cast<int*>(a) = 1;
    *static_cast<int*>(b) = 2;
    *static_cast<int*>(c) = 3;

    stack.reset();
}

// Lifecycle
// shows object construction and destruction using create<T>() and destroy<T>()
void lifecycle() {
    struct Vec3 {
        float x, y, z;
        Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
        ~Vec3() {}
    };

    StackAllocator stack(256);

    Vec3* v = stack.create<Vec3>(1.0f, 2.0f, 3.0f);
    stack.destroy(v);
}

// Marker Rewind
// shows getMarker() and freeToMarker() for partial stack rewind
void markerRewind() {
    StackAllocator stack(256);

    void* a = stack.allocate(sizeof(int), alignof(int));
    void* b = stack.allocate(sizeof(int), alignof(int));
    (void)a; (void)b;

    std::size_t marker = stack.getMarker();

    void* c = stack.allocate(sizeof(int), alignof(int));
    void* d = stack.allocate(sizeof(int), alignof(int));
    (void)c; (void)d;

    // Rewind to saved marker — c and d are discarded
    stack.freeToMarker(marker);

    stack.reset();
}

// Stack Scope
// shows automatic rewind on scope exit via StackScope
void stackScope() {
    StackAllocator stack(256);

    void* a = stack.allocate(sizeof(int), alignof(int));
    (void)a;

    {
        StackScope scope(stack);

        void* b = stack.allocate(sizeof(int), alignof(int));
        void* c = stack.allocate(sizeof(int), alignof(int));
        (void)b; (void)c;

        // scope exits here — stack rewinds to marker saved at StackScope construction
    }

    stack.reset();
}

// Reset
// shows reset() wiping the stack for reuse
void reset() {
    StackAllocator stack(256);

    void* a = stack.allocate(sizeof(int), alignof(int));
    void* b = stack.allocate(sizeof(int), alignof(int));
    (void)a; (void)b;

    stack.reset();

    // Stack is empty — full capacity available again
    void* c = stack.allocate(sizeof(int), alignof(int));
    (void)c;

    stack.reset();
}

// Owns
// shows pointer ownership boundary checks using owns()
void owns() {
    StackAllocator stack(256);

    void* inside  = stack.allocate(sizeof(int), alignof(int));
    int   outside = 0;

    bool a = stack.owns(inside);   // true
    bool b = stack.owns(&outside); // false
    bool c = stack.owns(nullptr);  // false

    (void)a; (void)b; (void)c;

    stack.reset();
}

// Out of Memory
// shows graceful nullptr return when the stack is exhausted
void oom() {
    StackAllocator stack(2 * sizeof(int));

    void* a = stack.allocate(sizeof(int), alignof(int)); // ok
    void* b = stack.allocate(sizeof(int), alignof(int)); // ok
    void* c = stack.allocate(sizeof(int), alignof(int)); // nullptr — stack exhausted

    (void)c;

    // Reset and allocate again
    stack.reset();
    void* d = stack.allocate(sizeof(int), alignof(int)); // ok
    (void)d;

    stack.reset();
}

// Move
// shows move construction and move assignment, verifying source is zeroed
void move() {
    // Move construction
    {
        StackAllocator stack(256);
        void* p = stack.allocate(sizeof(int), alignof(int));
        (void)p;

        StackAllocator stack2(std::move(stack));
        // stack  : capacity = 0, used = 0 (zeroed)
        // stack2 : owns the buffer, p still valid
    }

    // Move assignment
    {
        StackAllocator stack(256);
        void* p = stack.allocate(sizeof(int), alignof(int));
        (void)p;

        StackAllocator stack2(128);
        stack2 = std::move(stack);
        // stack  : capacity = 0, used = 0 (zeroed)
        // stack2 : owns the buffer, p still valid
    }
}

// Entry Point
int main() {
    basicAllocation();
    lifecycle();
    markerRewind();
    stackScope();
    reset();
    owns();
    oom();
    move();

    return 0;
}