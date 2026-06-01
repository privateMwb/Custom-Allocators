#include <iostream>
#include <vector>
#include <chrono>

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

constexpr size_t count = 1'000'000;

auto clock_now() {
    return std::chrono::steady_clock::now();
}

auto stack_allocation() {
    StackAllocator stack(count * 32);
    
    auto start = clock_now();
    
    for(size_t i = 0; i<count; ++i) {
        stack.allocate(sizeof(int));
    }
    
    auto end = clock_now();
    
    return std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
}

auto new_allocation() {
    std::vector<int*> ptrs;
    
    ptrs.reserve(count);
    
    auto start = clock_now();
    
    for(size_t i = 0; i<count; ++i) {
        ptrs.push_back(new int);
    }
    
    auto end = clock_now();
    
    for(int* ptr : ptrs) {
        delete ptr;
    }
    
    return std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
}

void allocation_speed() {
    auto s_duration = stack_allocation();
    auto n_duration = new_allocation();
    
    cout << "Allocation Speed Benchmark\n\n";
    cout << "Stack Allocation: " << s_duration << "ms\n";
    cout << "New Allocation: " << n_duration << "ms\n\n";
}

auto stack_rollback() {
    StackAllocator stack(count * 32);
    
    auto start = clock_now();
    
    for(size_t i = 0; i<count; ++i) {
        size_t marker = stack.getMarker();
        
        stack.allocate(sizeof(int));
        stack.allocate(sizeof(int));
        stack.allocate(sizeof(int));
        
        stack.freeToMarker(marker);
    }
    
    auto end = clock_now();
    
    return std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
}

auto new_rollback() {
    auto start = clock_now();
    
    for(size_t i = 0; i<count; ++i) {
        int* a = new int;
        int* b = new int;
        int* c = new int;
        
        delete a;
        delete b;
        delete c;
    }
    
    auto end = clock_now();
    
    return std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
}

void marker_rollback_speed() {
    auto s_duration = stack_rollback();
    auto n_duration = new_rollback();
    
    cout << "Marker Rollback Speed Benchmark\n\n";
    cout << "Stack Rollback: " << s_duration << "ms\n";
    cout << "New Rollback: " << n_duration << "ms\n\n";
}

auto stack_object_creation() {
    StackAllocator stack(count * 32);
    
    auto start = clock_now();
    
    for(size_t i = 0; i < count; ++i) {
        stack.create<Object>(
            static_cast<int>(i),
            static_cast<int>(i+1),
            static_cast<int>(i+2)
        );
    }
    
    auto end = clock_now();
    
    return std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
}

auto new_object_creation() {
    std::vector<Object*> ptrs;
    
    ptrs.reserve(count);
    
    auto start = clock_now();
    
    for(size_t i = 0; i<count; ++i) {
        ptrs.push_back(new Object(
            static_cast<int>(i),
            static_cast<int>(i+1),
            static_cast<int>(i+2)
        ));
    }
    
    auto end = clock_now();
    
    for(Object* ptr : ptrs) {
        delete ptr;
    }
    
    return std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
}

void object_creation_speed() {
    auto s_duration = stack_object_creation();
    auto n_duration = new_object_creation();
    
    cout << "Object Creation Speed Benchmark\n\n";
    cout << "Stack Object Creation: " << s_duration << "ms\n";
    cout << "New Object Creation: " << n_duration << "ms\n\n";
}

int main() {
    allocation_speed();
    
    marker_rollback_speed();
    
    object_creation_speed();
    
    return 0;
}