#include <iostream>
#include <cstdint>

#include "FreeListAllocator.h"

using std::cout;
using std::size_t;

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

void display(FreeListAllocator& fl) {
    cout << "Used: " << fl.used() << "\n";
    cout << "Remaining: " << fl.remaining() << "\n";
    cout << "Capacity: " << fl.capacity() << "\n\n";
}

void basic_allocation() {
    FreeListAllocator fl(256);
    
    cout << "Basic Allocation Example\n\n";
    cout << "Default:\n";
    display(fl);
    
    void* a = fl.allocate(16);
    void* b = fl.allocate(32);
    void* c = fl.allocate(8);
    
    cout << "After Allocation:\n";
    display(fl);
    
    cout << "a: " << a << "\n";
    cout << "b: " << b << "\n";
    cout << "c: " << c << "\n\n";
}

void reuse_usage() {
    FreeListAllocator fl(256);
    
    cout << "Reuse Usage Example\n\n";
    cout << "Default:\n";
    display(fl);
    
    void* a = fl.allocate(16);
    
    cout << "After First Allocation:\n";
    display(fl);
    
    cout << "a: " << a << "\n\n";
    
    fl.deallocate(a);

    void* b = fl.allocate(16);
    
    cout << "After Second Allocation:\n";
    display(fl);
    
    cout << "b: " << b << "\n\n";
}

void object_creation_usage() {
    FreeListAllocator fl(256);
    
    cout << "Object Creation Example\n\n";
    cout << "Default:\n";
    display(fl);
    
    Object* obj = fl.create<Object>(12, 89, 99);
    
    cout << "After Object Creation:\n";
    display(fl);
    
    cout << "obj->x: " << obj->x << "\n";
    cout << "obj->y: " << obj->y << "\n";
    cout << "obj->z: " << obj->z << "\n\n";
    
    fl.destroy(obj);
    
    cout << "After Object Destroy\n";
    display(fl);
}

void alignment_usage() {
    size_t alignment = alignof(SIMDObject);
    
    FreeListAllocator fl(256, alignment);
    
    cout << "Alignment Usage Example\n\n";
    cout << "Default:\n";
    display(fl);
    
    SIMDObject* obj = fl.create<SIMDObject>();
    
    uintptr_t address = reinterpret_cast<uintptr_t>(obj);
    
    cout << "After Aligned Allocation:\n";
    display(fl);
    
    cout << "address: " << obj << "\n";
    cout << "address % " << alignment << " == 0: " << 
    (address % alignment == 0 ? "True" : "False") << "\n\n";
}

void coalesce_usage() {
    FreeListAllocator fl(256);
    
    cout << "Coalesce Usage Example\n\n";
    cout << "Default:\n";
    display(fl);
    
    void* a = fl.allocate(32);
    void* b = fl.allocate(32);
    
    cout << "After Allocation:\n";
    display(fl);
    
    fl.deallocate(a);
    fl.deallocate(b);
    
    cout << "After Coalescing:\n";
    display(fl);
    
    void* c = fl.allocate(64);
    
    cout << "After Allocation:\n";
    display(fl);
    
    cout << "c: " << c << "\n\n";
}

int main() {
    basic_allocation();
    
    reuse_usage();
    
    object_creation_usage();
    
    alignment_usage();
    
    coalesce_usage();
    
    return 0;
}