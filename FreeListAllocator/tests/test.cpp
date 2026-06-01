#include <iostream>
#include <cassert>

#include "FreeListAllocator.h"

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
    FreeListAllocator fl(128);
    
    void* a = fl.allocate(16);
    
    size_t before = fl.used();
    
    void* b = fl.allocate(32);
    
    assert(a != nullptr && b != nullptr);
    
    assert(fl.used() > before);
    
    assert(fl.remaining() < fl.capacity());
    
    cout << "Basic Test Completed\n\n";
}

void test_reuse() {
    FreeListAllocator fl(128);
    
    void* a = fl.allocate(16);
    
    fl.deallocate(a);
    
    void* b = fl.allocate(16);
    
    assert(b != nullptr);

    cout << "Reuse Test Completed\n\n";
}

void test_alignment() {
    size_t alignment = alignof(SIMDObject);
    
    FreeListAllocator fl(sizeof(SIMDObject) * 2, alignment);
    
    SIMDObject* obj = fl.create<SIMDObject>();
    
    assert(obj != nullptr);
    
    uintptr_t address = reinterpret_cast<uintptr_t>(obj);
    
    assert(address % alignment == 0);
    
    fl.destroy(obj);
    
    cout << "Alignment Test Completed\n\n";
}

void test_create_destroy() {
    FreeListAllocator fl(128);
    
    int alive = 0;
    
    Object* obj = fl.create<Object>(alive);
    
    assert(obj != nullptr);
    assert(alive == 1);
    
    fl.destroy(obj);
    
    assert(alive == 0);
    
    cout << "Create/Destroy Test Completed\n\n";
}

void test_exhaustion() {
    FreeListAllocator fl(128);
    
    int count = 0;
    
    while(fl.allocate(8)) {
        ++count;
    }
    
    assert(count > 0);
    
    void* a = fl.allocate(8);
    
    assert(a == nullptr);

    cout << "Exhaustion Test Completed\n\n";
}

void test_coalesce() {
    FreeListAllocator fl(256);
    
    void* a = fl.allocate(32);
    void* b = fl.allocate(32);
    
    fl.deallocate(a);
    fl.deallocate(b);
    
    void* c = fl.allocate(64);
    
    assert(c != nullptr);
    
    cout << "Coalesce Test Completed\n\n";
}

int main() {
    test_basic();
    
    test_reuse();
    
    test_alignment();
    
    test_create_destroy();
    
    test_exhaustion();
    
    test_coalesce();
    
    
    return 0;
}