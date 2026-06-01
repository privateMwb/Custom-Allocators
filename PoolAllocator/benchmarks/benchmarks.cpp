#include <iostream>
#include <cstdint>
#include <chrono>
#include <vector>

#include "PoolAllocator.h"

using std::size_t;
using std::cout;

constexpr size_t count = 1'000'000;


auto clock_now() {
	return std::chrono::steady_clock::now();
}

auto pool_allocation() {
	PoolAllocator pool(sizeof(int), count);

	auto start = clock_now();

	for(size_t i = 0; i<count; ++i) {
		pool.allocate();
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
	auto p_duration = pool_allocation();

	auto n_duration = new_allocation();

	cout << "Allocation Speed Benchmarks\n";
	cout << "Pool Allocation: " << p_duration << "ms\n";
	cout << "New Allocation: " << n_duration << "ms\n\n";
}

auto pool_reuse() {
	PoolAllocator pool(sizeof(int), count);

	auto start = clock_now();

	for(size_t i = 0; i<count; ++i) {
		void* x = pool.allocate();

		pool.deallocate(x);
	}

	auto end = clock_now();

	return std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
}

auto new_reuse() {
	auto start = clock_now();

	for(size_t i = 0; i<count; ++i) {
		int* ptr = new int;

		delete ptr;
	}

	auto end = clock_now();

	return std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
}

void reuse_speed() {
	auto p_duration = pool_reuse();
	auto n_duration = new_reuse();

	cout << "Reuse Speed Benchmarks\n";
	cout << "Pool Reuse: " << p_duration << "ms\n";
	cout << "New/Delete Reuse: " << n_duration << "ms\n\n";
}

template<typename T>
auto pool_object_creation() {
	PoolAllocator pool(sizeof(T), count);

	auto start = clock_now();

	for(size_t i = 0; i<count; ++i) {
		pool.create<T>(
		    static_cast<int>(i),
		    static_cast<int>(i+1),
		    static_cast<int>(i+2)
		);
	}

	auto end = clock_now();

	return std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
}

template<typename T>
auto new_object_creation() {
	std::vector<T*> ptrs;

	ptrs.reserve(count);

	auto start = clock_now();

	for(size_t i = 0; i<count; ++i) {
		ptrs.push_back(new T(
		                   static_cast<int>(i),
		                   static_cast<int>(i+1),
		                   static_cast<int>(i+2)
		               )
		              );
	}

	auto end = clock_now();

	for(T* ptr : ptrs) {
		delete ptr;
	}

	return std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
}

void object_creation_speed() {
	struct Object {
		int x, y, z;

		Object(int a, int b, int c) :
			x(a),
			y(b),
			z(c) {}
	};

	auto p_duration = pool_object_creation<Object>();
	auto n_duration = new_object_creation<Object>();

	cout << "Object Creation Speed Benchmarks\n";
	cout << "Pool Create: " << p_duration << "ms\n";
	cout << "New Create: " << n_duration << "ms\n\n";
}

int main() {
	allocation_speed();

	reuse_speed();

	object_creation_speed();

	return 0;
}