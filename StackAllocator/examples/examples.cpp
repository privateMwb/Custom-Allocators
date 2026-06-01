#include <iostream>
#include <cstdint>

#include "StackAllocator.h"
#include "StackScope.h"

using std::cout;
using std::size_t;

struct alignas(32) SIMDObject {
    float data[8];
};

struct Object {
    int x, y, z;
    
    Object(int a, int b, int c) :
    x(a),
    y(b),
    z(c) {}
};

void display(StackAllocator& stack) {
    cout << "Used: " << stack.used() << "\n";
    cout << "Remaining: " << stack.remaining() << "\n";
    cout << "Capacity: " << stack.capacity() << "\n\n";
}

void basic_allocation() {
    StackAllocator stack(128);
    
    cout << "Basic Allocation Example\n\n";
    cout << "Default:\n";
    display(stack);
    
    void* a = stack.allocate(16);
    void* b = stack.allocate(16);
    void* c = stack.allocate(16);
    
    cout << "After Allocation:\n";
    display(stack);
    
    cout << "a: " << a << "\n";
    cout << "b: " << b << "\n";
    cout << "c: " << c << "\n\n";
}

void marker_usage() {
    StackAllocator stack(128);
    
    cout << "Marker Usage Example\n\n";
    cout << "Default:\n";
    display(stack);
    
    stack.allocate(16);
    stack.allocate(16);
    
    size_t marker = stack.getMarker();
    
    stack.allocate(32);
    
    cout << "After Temporary Allocation:\n";
    display(stack);
    
    stack.freeToMarker(marker);
    
    cout << "After Marker Rollback:\n";
    display(stack);
}

void object_creation() {
    StackAllocator stack(128);
    
    cout << "Object Creation Example\n\n";
    cout << "Default:\n";
    display(stack);
    
    Object* obj = stack.create<Object>(45, 78, 98);
    
    cout << "After Object Allocation:\n";
    display(stack);
    
    cout << "obj->x: " << obj->x << "\n";
    cout << "obj->y: " << obj->y << "\n";
    cout << "obj->z: " << obj->z << "\n\n";
    
    stack.destroy(obj);
    
    cout << "After Object Destroy:\n";
    display(stack);
}

void scope_usage() {
    StackAllocator stack(128);

    cout << "Scope Usage Example\n\n";
    cout << "Default:\n";
    display(stack);
    
    stack.allocate(16);
    
    {
        StackScope scope(stack);
        
        stack.allocate(32);
        stack.allocate(18);
        
        cout << "During Scope:\n";
        display(stack);
    }
    
    cout << "After Scope:\n";
    display(stack);
}

void alignment_usage() {
    size_t alignment = alignof(SIMDObject);
    
    StackAllocator stack(sizeof(SIMDObject) * 2, alignment);
    
    cout << "Alignment Usage Example\n\n";
    cout << "Default:\n";
    display(stack);
    
    SIMDObject* obj = stack.create<SIMDObject>();
    
    uintptr_t address = reinterpret_cast<uintptr_t>(obj);
    
    cout << "After Aligned Allocation:\n";
    display(stack);
    
    cout << "Address: " << obj << "\n";
    cout << "Address % " << alignment << " == 0: " << 
    (address % alignment == 0 ? "True" : "False") << "\n\n";
}

void clear_usage() {
    StackAllocator stack(128);
    
    cout << "Clear Usage Example\n\n";
    cout << "Default:\n";
    display(stack);
    
    stack.allocate(16);
    stack.allocate(16);
    stack.allocate(16);
    
    cout << "After Allocation:\n";
    display(stack);
    
    stack.clear();
    
    cout << "After Clear:\n";
    display(stack);
}

int main() {
    basic_allocation();
    
    marker_usage();
    
    object_creation();
    
    scope_usage();
    
    alignment_usage();
    
    clear_usage();
    
    return 0;
}