#include <iostream>
#include <cassert>
#include <cstdint>

#include "PoolAllocator.h"

using std::size_t;
using std::cout;

void test_basic() {
    PoolAllocator pool(1024, 5);
    
    assert(pool.freeBlocks() == 5);
    assert(pool.usedBlocks() == 0);
    
    void* a = pool.allocate();
    void* b = pool.allocate();
    
    assert(a != nullptr);
    assert(b != nullptr);
    
    assert(pool.freeBlocks() == 3);
    assert(pool.usedBlocks() == 2);
    
    pool.deallocate(a);
    pool.deallocate(b);
    
    assert(pool.freeBlocks() == 5);
    assert(pool.usedBlocks() == 0);
    
    cout << "Basic Test Completed\n\n";
}

void test_reuse() {
    PoolAllocator pool(1024, 1);
    
    void* a = pool.allocate();
    
    assert(a != nullptr);
    
    pool.deallocate(a);
    
    void* b = pool.allocate();
    
    assert(b != nullptr);
    
    assert(a == b);
    
    cout << "Reuse Test Completed\n\n";
}

void test_alignment() {
    struct alignas(32) SIMDObject {
        float data[8];
    };
    
    PoolAllocator pool(sizeof(SIMDObject), 4, 32);
    
    SIMDObject* obj = pool.create<SIMDObject>();
    
    assert(obj != nullptr);
    
    uintptr_t address = reinterpret_cast<uintptr_t>(obj);
    
    assert(address % 32 == 0);
    
    cout << "Alignment Test Completed\n\n";
}

void test_create_destroy() {

    static int alive = 0;

    struct Object {

        int& counter;

        Object(int& c) : counter(c) {
            ++counter;
        }

        ~Object() {
            --counter;
        }
    };

    PoolAllocator pool(sizeof(Object), 2);

    assert(alive == 0);

    Object* obj = pool.create<Object>(alive);

    assert(alive == 1);

    pool.destroy(obj);

    assert(alive == 0);
    
    assert(pool.freeBlocks() == 2);

    cout << "Create/Destroy Test Passed\n\n";
}

void test_exhaustion() {
    PoolAllocator pool(64, 2);
    
    void* a = pool.allocate();
    void* b = pool.allocate();
    void* c = pool.allocate();
    
    assert(a != nullptr);
    assert(b != nullptr);
    assert(c == nullptr);
    
    assert(pool.usedBlocks() == 2);
    assert(pool.freeBlocks() == 0);
    
    cout << "Exhaustion Test Completed\n\n";
}

int main() {
    test_basic();
    
    test_reuse();
    
    test_alignment();
    
    test_create_destroy();
    
    test_exhaustion();
    
    return 0;
}