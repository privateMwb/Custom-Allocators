#include <iostream>
#include <chrono>
#include <vector>

#include "FreeListAllocator.h"

using std::cout;
using std::size_t;

constexpr size_t count = 1'000'000;

struct Object {
    int x, y, z;
    
    Object(int a, int b, int c) :
    x(a),
    y(b),
    z(c) {}
};

auto clock_now() {
    return std::chrono::steady_clock::now();
}

auto free_list_allocation() {
    FreeListAllocator fl(count * 32);
    
    auto start = clock_now();
    
    for(size_t i = 0; i<count; ++i) {
        fl.allocate(sizeof(int));
    }
    
    auto end = clock_now();
    
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
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
    
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

void allocation_speed() {
    auto f_duration = free_list_allocation();
    auto n_duration = new_allocation();
    
    cout << "Allocation Speed Benchmark\n\n";
    cout << "Free List Allocation: " << f_duration << "ms\n";
    cout << "New Allocation: " << n_duration << "ms\n\n";
}

auto free_list_reuse() {
    FreeListAllocator fl(count * 32);
    
    auto start = clock_now();
    
    for(size_t i = 0; i<count; ++i) {
        void* a = fl.allocate(sizeof(int));
        void* b = fl.allocate(sizeof(int));
        void* c = fl.allocate(sizeof(int));
        
        fl.deallocate(a);
        fl.deallocate(b);
        fl.deallocate(c);
    }
    
    auto end = clock_now();
    
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

auto new_reuse() {
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
    
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

void reuse_speed() {
    auto f_duration = free_list_reuse();
    auto n_duration = new_reuse();
    
    cout << "Reuse Speed Benchmark\n\n";
    cout << "Free List Reuse: " << f_duration << "ms\n";
    cout << "New Reuse: " << n_duration << "ms\n\n";
}

auto free_list_object_creation() {
    FreeListAllocator fl(count * 32);
    
    auto start = clock_now();
    
    for(size_t i = 0; i<count; ++i) {
        fl.create<Object>(
            static_cast<int>(i),
            static_cast<int>(i+1),
            static_cast<int>(i+2)
        );
    }
    
    auto end = clock_now();
    
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
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
            )
        );
    }
    
    auto end = clock_now();
    
    for(Object* ptr : ptrs) {
        delete ptr;
    }
    
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

void object_creation_speed() {
    auto f_duration = free_list_object_creation();
    auto n_duration = new_object_creation();
    
    cout << "Object Creation Speed Benchmark\n\n";
    cout << "Free List Object Creation: " << f_duration << "ms\n";
    cout << "New Object Creation: " << n_duration << "ms\n\n";
}

int main() {
    allocation_speed();
    
    reuse_speed();
    
    object_creation_speed();
    
    return 0;
}