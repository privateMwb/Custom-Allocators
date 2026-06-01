#include <iostream>
#include <chrono>
#include <vector>

#include "ArenaAllocator.h"
#include "ArenaScope.h"

using std::cout;
using std::size_t;

constexpr size_t count = 1'000'000;
constexpr size_t frcnt = 10'000;
constexpr size_t frame = 1'000;

struct Object {
	int x, y, z;

	Object(int a, int b, int c) : x(a), y(b), z(c) {}
};

auto clock_now() {
	return std::chrono::steady_clock::now();
}

auto arenaAllocation() {
	ArenaAllocator arena(count * sizeof(int));

	auto start = clock_now();

	for(size_t i = 0; i<count; ++i) {
		arena.allocate<int>();
	}

	auto end = clock_now();

	return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

auto newDeleteAllocation() {
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

void allocationSpeed() {
	auto a_duration = arenaAllocation();
	auto s_duration = newDeleteAllocation();

	cout << "Allocation Speed Benchmark\n";
	cout << "Arena Allocation: " << a_duration << "ms\n";
	cout << "New/Delete: " << s_duration << "ms\n\n";
}

auto arenaReset() {
	ArenaAllocator arena(count * sizeof(int));

	for(size_t i = 0; i<count; ++i) {
		arena.allocate<int>();
	}

	auto start = clock_now();

	arena.reset();

	auto end = clock_now();

	return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

auto deleteLoop() {
	std::vector<int*> ptrs;

	ptrs.reserve(count);

	for(size_t i=0; i<count; ++i) {
		ptrs.push_back(new int);
	}

	auto start = clock_now();

	for(int* ptr : ptrs) {
		delete ptr;
	}

	auto end = clock_now();

	return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

void cleanupSpeed() {
	auto a_duration = arenaReset();
	auto s_duration = deleteLoop();

	cout << "Cleanup Speed Benchmark\n";
	cout << "Arena Reset: " << a_duration << "ms\n";
	cout << "Delete Loop: " << s_duration << "ms\n\n";
}


auto arenaObjectCreation() {
	ArenaAllocator arena(count * sizeof(Object));

	auto start = clock_now();

	for(size_t i=0; i<count; ++i) {
		arena.create<Object>(
		    static_cast<int>(i),
		    static_cast<int>(i+1),
		    static_cast<int>(i+3)
		);
	}

	auto end = clock_now();

	return std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
}

auto newObjectCreation() {
	std::vector<Object*> objects;

	objects.reserve(count);

	auto start = clock_now();

	for(size_t i = 0; i<count; ++i) {
		objects.push_back(
		    new Object(
		        static_cast<int>(i),
		        static_cast<int>(i+1),
		        static_cast<int>(i+3)
		    )
		);
	}

	auto end = clock_now();
	
	for(Object* obj : objects) {
	    delete obj;
	}
	
	return std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
}

void objectLifecycleSpeed() {
    auto a_duration = arenaObjectCreation();
    auto s_duration = newObjectCreation();
    
    cout << "Object Lifecycle Speed Benchmark\n";
    cout << "Arena Create<T>: " << a_duration << "ms\n";
    cout << "New Object: " << s_duration << "ms\n\n";
}

auto arenaFrameStress() {
    ArenaAllocator arena(frcnt * sizeof(int) * 2);
    
    auto start = clock_now();
    
    for(size_t i = 0; i<frame; ++i) {
        
        arena.beginFrame();
        
        for(size_t j = 0; j<frcnt; ++j) {
            arena.allocate<int>();
        }
        
        arena.endFrame();
    }
    
    auto end = clock_now();
    
    return std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
}

auto deleteLoopStress() {
    std::vector<int*> ptrs;
    
    
    auto start = clock_now();
    
    for(size_t i = 0; i<frame; ++i) {
        ptrs.clear();
        ptrs.reserve(frcnt);
        
        for(size_t j = 0; j<frcnt; ++j) {
            ptrs.push_back(new int);
        }
        
        for(int* ptr : ptrs) {
            delete ptr;
        }
    }
    
    auto end = clock_now();
    
    return std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
}

void frameSpeed() {
    auto a_duration = arenaFrameStress();
    auto s_duration = deleteLoopStress();
    
    cout << "Frame Speed Benchmark\n";
    cout << "Arena Frame: " << a_duration << "ms\n";
    cout << "Delete Frame: " << s_duration << "ms\n\n";
}

int main() {
	allocationSpeed();

	cleanupSpeed();
	
	objectLifecycleSpeed();
	
	frameSpeed();
	
	return 0;
}