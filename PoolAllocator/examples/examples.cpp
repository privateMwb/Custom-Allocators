#include <iostream>
#include <cstdint>

#include "PoolAllocator.h"

using std::size_t;
using std::cout;

struct Object {
    int x, y, z;
    
    Object(int a, int b, int c) :
    x(a),
    y(b),
    z(c) {}
};

struct alignas(32) SIMDObject {
    float data[8];
};

void display(PoolAllocator& pool) {
    cout << "Capacity: " << pool.totalBlocks() << "\n";
    cout << "Used Blocks: " << pool.usedBlocks() << "\n";
    cout << "Free Blocks: " << pool.freeBlocks() << "\n\n";
}

void basic_allocation() {
    PoolAllocator pool(sizeof(int), 5);
    
    cout << "Basic Allocation Example\n\n";
    cout << "Default:\n";
    display(pool);
    
    int* a = static_cast<int*>(pool.allocate());
    int* b = static_cast<int*>(pool.allocate());
    
    *a = 42;
    *b = 24;
    
    cout << "After Allocation:\n";
    display(pool);
    
    cout << "a: " << *a << "\n";
    cout << "b: " << *b << "\n\n";
    
    pool.deallocate(a);
    pool.deallocate(b);
    
    cout << "After Deallocation:\n";
    display(pool);
}

void object_creation() {
    PoolAllocator pool(sizeof(Object), 10);
    
    cout << "Object Creation Example\n\n";
    cout << "Default:\n";
    display(pool);
    
    Object* obj = pool.create<Object>(23, 67, 99);
    
    cout << "After Object Creation:\n";
    display(pool);
    
    cout << "obj->x: " << obj->x << "\n";
    cout << "obj->y: " << obj->y << "\n";
    cout << "obj->z: " << obj->z << "\n\n";
    
    pool.destroy(obj);
    
    cout << "After Object Destroy:\n";
    display(pool);
}

void pool_reuse() {
    PoolAllocator pool1(sizeof(int), 1);
    
    cout << "Reuse Example\n\n";
    
    cout << "Block Pool Default:\n";
    display(pool1);
    
    int* a = static_cast<int*>(pool1.allocate());
    *a = 45;
        
    pool1.deallocate(a);
    
    int* b = static_cast<int*>(pool1.allocate());
    *b = 79;
    
    cout << "After Block Reuse:\n";
    display(pool1);
    
    cout << "a address: " << a << "\n";
    cout << "b address: " << b << "\n";
    
    cout << "a == b: " << (a == b ? "True" : "False") << "\n\n";
    
    pool1.deallocate(b);
    
    PoolAllocator pool2(sizeof(Object), 1);
    
    cout << "Object Pool Default:\n";
    display(pool2);
    
    Object* obj1 = pool2.create<Object>(21, 33, 45);
    
    pool2.destroy(obj1);
        
    Object* obj2 = pool2.create<Object>(41, 56, 15);
    
    cout << "After Object Reuse:\n";
    display(pool2);
    
    cout << "obj1 address: " << obj1 << "\n";
    cout << "obj2 address: " << obj2 << "\n";
    
    cout << "obj1 == obj2: " << (obj1 == obj2 ? "True" : "False") << "\n\n";
    
    pool2.destroy(obj2);
}

void alignment_usage() {
    constexpr size_t alignment = alignof(SIMDObject);
    
    PoolAllocator pool(sizeof(SIMDObject), 3, alignment);
    
    cout << "Alignment Usage Example\n\n";
    cout << "Default:\n";
    display(pool);
    
    SIMDObject* obj = pool.create<SIMDObject>();
    
    uintptr_t address = reinterpret_cast<uintptr_t>(obj);
    
    cout << "After Aligned Allocation:\n";
    display(pool);
    
    cout << "Address: " << obj << "\n";
    cout << "address % " << alignment << " == 0: " << (address % alignment == 0 ? "True" : "False") << "\n\n";
}

void exhaustion_handling() {
    PoolAllocator pool(sizeof(int), 2);
    
    cout << "Exhaustion Handling Example\n\n";
    cout << "Default:\n";
    display(pool);
    
    void* a = pool.allocate();
    void* b = pool.allocate();
    
    cout << "After Full Allocation:\n";
    display(pool);
    
    void *c = pool.allocate();
    
    cout << "Third Allocation: " << (c == nullptr ? "Failed" : "Success") << "\n\n";
    
    pool.deallocate(a);
    pool.deallocate(b);
    
    cout << "After Recovery:\n";
    display(pool);
}

int main() {
	basic_allocation();
	
	object_creation();
	
	pool_reuse();
	
	alignment_usage();
	
	exhaustion_handling();
	
	return 0;
}