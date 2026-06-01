#include <iostream>
#include <cassert>

#include "StackAllocator.h"
#include "StackScope.h"

using std::cout;
using std::size_t;

struct alignas(32) SIMDObject {
    float data[8];
};

struct Object {
    int& alive;
    
    Object(int& counter) :
    alive(counter) {
        ++alive;
    }
    
    ~Object() {
        --alive;
    }
};

void test_basic() {
    StackAllocator stack(32);
    
    void* a = stack.allocate(2);
    void* b = stack.allocate(6);
    
    assert(a != nullptr && b != nullptr);
    
    assert(stack.used() > 0);
    assert(stack.remaining() < stack.capacity());
    
    cout << "Basic Test Completed\n\n";
}

void test_marker() {
    StackAllocator stack(64);
    
    stack.allocate(5);
    
    size_t marker = stack.getMarker();
    size_t before = stack.used();
    
    stack.allocate(10);
    
    stack.freeToMarker(marker);
    
    assert(stack.used() == before);
    
    cout << "Marker Test Completed\n\n";
}

void test_nested_markers() {
    StackAllocator stack(128);
    
    size_t m1 = stack.getMarker();
    size_t b1 = stack.used();
    
    stack.allocate(34);
    
    size_t m2 = stack.getMarker();
    size_t b2 = stack.used();
    
    stack.allocate(34);
    
    stack.freeToMarker(m2);
    
    assert(stack.used() == b2);
    
    stack.freeToMarker(m1);
    
    assert(stack.used() == b1);
    
    cout << "Nested Markers Test Completed\n\n";
}

void test_alignment() {
    size_t alignment = alignof(SIMDObject);
    
    StackAllocator stack(sizeof(SIMDObject) * 2, alignment);
    
    SIMDObject* obj = stack.create<SIMDObject>();
    
    uintptr_t address = reinterpret_cast<uintptr_t>(obj);
    
    assert(address % alignment == 0);
    
    stack.destroy(obj);
    
    cout << "Alignment Test Completed\n\n";
}

void test_create_destroy() {
    StackAllocator stack(sizeof(Object));
    
    int alive = 0; 
    
    Object* obj = stack.create<Object>(alive);
    
    assert(obj != nullptr);
    assert(alive == 1);
    
    stack.destroy(obj);
    
    assert(alive == 0);
    
    cout << "Create/Destroy Test Completed\n\n";
}

void test_clear() {
    StackAllocator stack(128);
    
    stack.allocate(32);
    
    size_t before = stack.used();
    
    stack.allocate(10);
    
    assert(stack.used() > before);
    
    stack.clear();
    
    assert(stack.used() == 0);
    assert(stack.remaining() == stack.capacity());
    
    cout << "Clear Test Completed\n\n";
}

void test_scope() {
    StackAllocator stack(128);
    
    stack.allocate(28);
    
    size_t before = stack.used();
    
    {
        StackScope scope(stack);
        
        stack.allocate(32);
        
        assert(stack.used() > before);
    }
    
    assert(stack.used() == before);
    
    cout << "Scope Test Completed\n\n";
}

void test_exhaustion() {
    StackAllocator stack(32);
    
    void* a = stack.allocate(16);
    void* b = stack.allocate(16);
    void* c = stack.allocate(1);
    
    assert(a != nullptr);
    assert(b != nullptr);
    assert(c == nullptr);
    
    cout << "Exhaustion Test Completed\n\n";
}

int main() {
    test_basic();
    
    test_marker();
    
    test_nested_markers();
    
    test_alignment();
    
    test_create_destroy();
    
    test_clear();
    
    test_scope();
    
    test_exhaustion();
    
    return 0;
}