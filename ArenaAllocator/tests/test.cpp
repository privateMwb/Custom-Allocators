#include <iostream>
#include <cassert>

#include "ArenaAllocator.h"
#include "ArenaScope.h"

using std::cout;

void test_basic() {
    ArenaAllocator arena(1024);
    
    int* x = arena.allocate<int>();
    *x = 42;
    
    assert(*x == 42);
    
    arena.reset();
    
    assert(arena.used() == 0);
    assert(arena.remaining() == arena.capacity());
    
    cout << "Test Basic Passed.\n";
}

void test_alignment() {
    ArenaAllocator arena(1024);
    
    int* x = arena.allocate<int>();
    double* y = arena.allocate<double>();
    
    assert(x != nullptr);
    assert(y != nullptr);
    
    assert(reinterpret_cast<size_t>(x) % alignof(int) == 0);
    assert(reinterpret_cast<size_t>(y) % alignof(double) == 0);
    
    cout << "Test Alignment Passed.\n";
}

void test_scope() {
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
    
    cout << "Test Scope Passed.\n";
}

void test_move() {
    ArenaAllocator arena(1024);
    
    int* x = arena.allocate<int>();
    
    assert(x != nullptr);
    
    *x = 42;
    
    ArenaAllocator arena2(std::move(arena));
    
    assert(arena2.used() == sizeof(int));
    assert(*x == 42);
    
    assert(arena.used() == 0);
    assert(arena.capacity() == 0);
    
    cout << "Test Move Passed.\n";
}

int main() {
    test_basic();
    
    test_alignment();
    
    test_scope();
    
    test_move();
    
    return 0;
}
