// StackAllocator Unit Test Suite
// Tests correctness of stack-based linear allocation and memory management:
//
// - basic allocation
// - exact stack utilization
// - out-of-memory handling
// - alignment guarantees
// - marker save and restore
// - free to marker rewind
// - object lifecycle (create/destroy)
// - reset behavior
// - ownership detection
// - allocation statistics
// - move semantics
// - scope-based RAII rewind
//
// These tests validate correctness of StackAllocator under normal
// allocation patterns and edge cases.

#include <iostream>
#include <cassert>

#include "StackAllocator.h"
#include "StackScope.h"

// Basic Allocation Test
// verifies successful allocation and writing to a single block
void basic() {
    StackAllocator stack(256);

    void* p = stack.allocate(sizeof(int), alignof(int));
    assert(p != nullptr);

    int* x = static_cast<int*>(p);
    *x = 42;
    assert(*x == 42);

    assert(stack.used()      == sizeof(int));
    assert(stack.remaining() == 256 - sizeof(int));
    assert(stack.capacity()  == 256);

    std::cout << "\n[PASS] Basic Test\n";
}

// Exact Fill Test
// verifies the stack can be filled to capacity
void exactFill() {
    StackAllocator stack(4 * sizeof(int));

    void* a = stack.allocate(sizeof(int), alignof(int));
    void* b = stack.allocate(sizeof(int), alignof(int));
    void* c = stack.allocate(sizeof(int), alignof(int));
    void* d = stack.allocate(sizeof(int), alignof(int));

    assert(a != nullptr);
    assert(b != nullptr);
    assert(c != nullptr);
    assert(d != nullptr);

    assert(stack.used()      == 4 * sizeof(int));
    assert(stack.remaining() == 0);

    std::cout << "\n[PASS] Exact Fill Test\n";
}

// Out of Memory Test
// verifies allocation returns nullptr when stack is exhausted
void oom() {
    StackAllocator stack(2 * sizeof(int));

    void* a = stack.allocate(sizeof(int), alignof(int));
    void* b = stack.allocate(sizeof(int), alignof(int));

    assert(a != nullptr);
    assert(b != nullptr);

    void* c = stack.allocate(sizeof(int), alignof(int));
    assert(c == nullptr);

    std::cout << "\n[PASS] Out of Memory Test\n";
}

// Alignment Test
// verifies returned pointers satisfy requested alignment
void alignment() {
    StackAllocator stack(256);

    void* p1 = stack.allocate(1,          1);
    void* p2 = stack.allocate(sizeof(int), alignof(int));
    void* p4 = stack.allocate(1,          4);
    void* p8 = stack.allocate(1,          8);

    assert(p1 != nullptr);
    assert(p2 != nullptr);
    assert(p4 != nullptr);
    assert(p8 != nullptr);

    assert(reinterpret_cast<std::uintptr_t>(p1) % 1  == 0);
    assert(reinterpret_cast<std::uintptr_t>(p2) % alignof(int) == 0);
    assert(reinterpret_cast<std::uintptr_t>(p4) % 4  == 0);
    assert(reinterpret_cast<std::uintptr_t>(p8) % 8  == 0);

    std::cout << "\n[PASS] Alignment Test\n";
}

// Marker Test
// verifies getMarker returns correct offset before and after allocations
void marker() {
    StackAllocator stack(256);

    std::size_t m0 = stack.getMarker();
    assert(m0 == 0);

    (void)stack.allocate(sizeof(int), alignof(int));
    std::size_t m1 = stack.getMarker();
    assert(m1 > m0);

    (void)stack.allocate(sizeof(int), alignof(int));
    std::size_t m2 = stack.getMarker();
    assert(m2 > m1);

    std::cout << "\n[PASS] Marker Test\n";
}

// Free To Marker Test
// verifies rewinding to a saved marker restores used() correctly
void freeToMarker() {
    StackAllocator stack(256);

    (void)stack.allocate(sizeof(int), alignof(int));
    std::size_t saved = stack.getMarker();

    (void)stack.allocate(sizeof(int), alignof(int));
    (void)stack.allocate(sizeof(int), alignof(int));

    assert(stack.used() > saved);

    stack.freeToMarker(saved);
    assert(stack.used() == saved);

    std::cout << "\n[PASS] Free To Marker Test\n";
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

    StackAllocator stack(256);

    bool destroyed = false;

    Foo* f = stack.create<Foo>(99, destroyed);
    assert(f != nullptr);
    assert(f->value == 99);
    assert(!destroyed);

    stack.destroy(f);
    assert(destroyed);

    std::cout << "\n[PASS] Lifecycle Test\n";
}

// Reset Test
// verifies reset() zeroes used() and allows fresh allocations
void reset() {
    StackAllocator stack(256);

    (void)stack.allocate(sizeof(int), alignof(int));
    (void)stack.allocate(sizeof(int), alignof(int));
    (void)stack.allocate(sizeof(int), alignof(int));

    assert(stack.used() > 0);

    stack.reset();

    assert(stack.used()      == 0);
    assert(stack.remaining() == 256);

    void* p = stack.allocate(sizeof(int), alignof(int));
    assert(p != nullptr);

    std::cout << "\n[PASS] Reset Test\n";
}

// Owns Test
// verifies pointer ownership detection inside the stack
void owns() {
    StackAllocator stack(256);

    void* inside  = stack.allocate(sizeof(int), alignof(int));
    int   outside = 0;

    assert( stack.owns(inside));
    assert(!stack.owns(&outside));
    assert(!stack.owns(nullptr));

    std::cout << "\n[PASS] Owns Test\n";
}

// Statistics Test
// verifies allocation statistics are tracked correctly
void stats() {
    StackAllocator stack(256);

    void* a = stack.allocate(sizeof(int), alignof(int));
    void* b = stack.allocate(sizeof(int), alignof(int));
    (void)a; (void)b;

    const auto& s = stack.getStats();

    assert(s.allocations    == 2);
    assert(s.totalAllocated >= sizeof(int) * 2);
    assert(s.currentUsed    == stack.used());
    assert(s.peakUsed       == stack.used());

    stack.freeToMarker(0);

    assert(s.currentUsed    == 0);
    assert(s.peakUsed       >= sizeof(int) * 2); // peak preserved after rewind

    std::cout << "\n[PASS] Stats Test\n";
}

// Move Semantics Test
// verifies move construction and move assignment transfer ownership
void move() {
    // Move construction
    {
        StackAllocator stack(256);

        (void)stack.allocate(sizeof(int), alignof(int));

        StackAllocator stack2(std::move(stack));

        assert(stack2.used()      >  0);
        assert(stack2.capacity()  == 256);

        assert(stack.used()      == 0);
        assert(stack.capacity()  == 0);
        assert(stack.remaining() == 0);
    }

    // Move assignment
    {
        StackAllocator stack(256);

        (void)stack.allocate(sizeof(int), alignof(int));

        StackAllocator stack2(128);
        stack2 = std::move(stack);

        assert(stack2.used()     >  0);
        assert(stack2.capacity() == 256);

        assert(stack.used()      == 0);
        assert(stack.capacity()  == 0);
        assert(stack.remaining() == 0);
    }

    std::cout << "\n[PASS] Move Test\n";
}

// Stack Scope Test
// verifies StackScope automatically rewinds the stack on destruction
void stackScope() {
    StackAllocator stack(256);

    (void)stack.allocate(sizeof(int), alignof(int));
    std::size_t before = stack.getMarker();

    {
        StackScope scope(stack);

        (void)stack.allocate(sizeof(int), alignof(int));
        (void)stack.allocate(sizeof(int), alignof(int));

        assert(stack.used() > before);
    }

    assert(stack.used() == before);

    std::cout << "\n[PASS] Stack Scope Test\n";
}

// Entry Point
int main() {
    basic();
    exactFill();
    oom();
    alignment();
    marker();
    freeToMarker();
    lifecycle();
    reset();
    owns();
    stats();
    move();
    stackScope();

    return 0;
}

