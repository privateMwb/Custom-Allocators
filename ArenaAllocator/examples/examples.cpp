#include <iostream>
#include <cstdint>

#include "ArenaAllocator.h"
#include "ArenaScope.h"

using std::cout;
using std::size_t;
using std::uintptr_t;

struct Object {
	int x, y, z;

	Object(int a, int b, int c) : x(a), y(b), z(c) {}
};

struct alignas(32) SIMDObject {
    float data[8];
};

void display(ArenaAllocator& arena) {
    cout << "Capacity: " << arena.capacity() << "\n";
    cout << "Used: " << arena.used() << "\n";
    cout << "Remaining: " << arena.remaining() << "\n\n";
}

void basic_allocation() {
    ArenaAllocator arena(1024);
    
    int* x = arena.allocate<int>();
    double* y = arena.allocate<double>();
    
    *x = 30;
    *y = 25.31;
    
    cout << "Basic Allocation Example\n\n";
    cout << "x: " << *x << "\n";
    cout << "y: " << *y << "\n\n";
    
    display(arena);
}

void object_creation() {
   ArenaAllocator arena(1024 * sizeof(Object));
   
   Object* obj = arena.create<Object>(1, 2, 3);
   
   cout << "Object Creation Example\n\n";
   cout << "obj->x: " << obj->x << "\n";
   cout << "obj->y: " << obj->y << "\n";
   cout << "obj->z: " << obj->z << "\n\n";
   
   display(arena);
}

void frame_usage() {
    ArenaAllocator arena(1024);
    
    cout << "Frame Usage Example\n\n";
    
    for(size_t frame = 0; frame<5; ++frame) {
        arena.beginFrame();
        
        for(size_t i = 0; i<100; ++i) {
            arena.allocate<int>();
        }
        
        cout << "Frame " << frame + 1 << ": \n";
        display(arena);
        
        arena.endFrame();
    }
}

void scope_usage() {
    ArenaAllocator arena(1024);
    
    {
        ArenaScope scope(arena);
        
        for(size_t i = 0; i<100; ++i) {
            arena.allocate<int>();
        }
        
        cout << "Scope Usage Example\n\n";
        display(arena);
    }
    
    cout << "After:\n";
    display(arena);
}

void alignment_usage() {
    ArenaAllocator arena(1024);
    
    SIMDObject* obj = arena.allocate<SIMDObject>();
    
    uintptr_t address = reinterpret_cast<uintptr_t>(obj);
    
    cout << "Alignment Usage Example\n\n";
    cout << "Address: " << address << "\n";
    cout << "32-byte aligned: " << (address % 32 == 0 ? "True" : "False") << "\n\n";
    
    display(arena);
}

int main() {
	basic_allocation();
	
	object_creation();
	
	frame_usage();
	
	scope_usage();
	
	alignment_usage();
	
	return 0;
}