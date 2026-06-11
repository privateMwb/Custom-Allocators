// StackAllocator Benchmark Suite
// Measures performance of stack-based linear allocation against traditional allocators:
//
// - single allocation (individual allocation cost)
// - bulk allocation (high-throughput allocation)
// - mixed allocation sizes (real-world allocation patterns)
// - marker rewind (partial stack rewind vs malloc/free)
// - churn (rapid allocate/reset cycles)
//
// Benchmarks compare StackAllocator with malloc/free and std::allocator.

#include <iostream>
#include <cstddef>
#include <chrono>
#include <memory>
#include <vector>

#include "StackAllocator.h"
#include "utils/Table.h"

// Config
static constexpr std::size_t SINGLE_ITERS = 100'000;
static constexpr std::size_t BULK_COUNT   = 1'000;
static constexpr std::size_t BULK_ITERS   = 1'000;
static constexpr std::size_t MIXED_ITERS  = 100'000;
static constexpr std::size_t MARKER_ITERS = 100'000;
static constexpr std::size_t CHURN_ITERS  = 100'000;

// returns elapsed microseconds for a callable
template<typename F>
auto duration(F func) {
	auto start = std::chrono::steady_clock::now();
	func();
	auto end   = std::chrono::steady_clock::now();
	return std::chrono::duration_cast<std::chrono::microseconds>(end - start);
}

// prevents the compiler from eliminating unused allocations
template<typename T>
inline void doNotOptimize(T* ptr) {
#if defined(__GNUC__) || defined(__clang__)
	asm volatile("" : : "r,m"(ptr) : "memory");
#else
	volatile T* v = ptr;
	(void)v;
#endif
}

// Mixed-size payload types
struct Small  {
	char data[4];
};
struct Medium {
	char data[16];
};
struct Large  {
	char data[64];
};

// Single Allocation
// measures pure allocation speed by filling the stack completely before resetting
void singleAllocation() {
	StackAllocator stack(SINGLE_ITERS * sizeof(int));

	auto stackTime = duration([&] {
		for (std::size_t i = 0; i < SINGLE_ITERS; ++i) {
			void* p = stack.allocate(sizeof(int), alignof(int));
			doNotOptimize(static_cast<int*>(p));
		}

		stack.reset();
	});

	auto mallocTime = duration([&] {
		std::vector<int*> ptrs;
		ptrs.reserve(SINGLE_ITERS);

		for (std::size_t i = 0; i < SINGLE_ITERS; ++i) {
			int* p = static_cast<int*>(std::malloc(sizeof(int)));
			doNotOptimize(p);
			ptrs.push_back(p);
		}

		for (int* p : ptrs) std::free(p);
	});

	std::allocator<int> stdAlloc;
	auto stdTime = duration([&] {
		std::vector<int*> ptrs;
		ptrs.reserve(SINGLE_ITERS);

		for (std::size_t i = 0; i < SINGLE_ITERS; ++i) {
			int* p = stdAlloc.allocate(1);
			doNotOptimize(p);
			ptrs.push_back(p);
		}

		for (int* p : ptrs) stdAlloc.deallocate(p, 1);
	});

	std::vector<std::string>              headers{ "Allocator", "Time (us)" };
	std::vector<std::vector<std::string>> data{
		{ "Stack", "malloc/free", "std::allocator" },
		{
			Table::format(stackTime.count(),  "us"),
			Table::format(mallocTime.count(), "us"),
			Table::format(stdTime.count(),    "us"),
		}
	};

	Table::table(
	    "Single Allocation  (" + std::to_string(SINGLE_ITERS) + " iters)",
	    headers, data, 44);
}

// Bulk Allocation
// measures filling and resetting the stack repeatedly vs malloc and std::allocator
void bulkAllocation() {
	StackAllocator stack(BULK_COUNT * sizeof(int));

	auto stackTime = duration([&] {
		for (std::size_t i = 0; i < BULK_ITERS; ++i) {
			for (std::size_t j = 0; j < BULK_COUNT; ++j) {
				void* p = stack.allocate(sizeof(int), alignof(int));
				doNotOptimize(static_cast<int*>(p));
			}

			stack.reset();
		}
	});

	auto mallocTime = duration([&] {
		for (std::size_t i = 0; i < BULK_ITERS; ++i) {
			std::vector<int*> ptrs;
			ptrs.reserve(BULK_COUNT);

			for (std::size_t j = 0; j < BULK_COUNT; ++j) {
				int* p = static_cast<int*>(std::malloc(sizeof(int)));
				doNotOptimize(p);
				ptrs.push_back(p);
			}

			for (int* p : ptrs) std::free(p);
		}
	});

	std::allocator<int> stdAlloc;
	auto stdTime = duration([&] {
		for (std::size_t i = 0; i < BULK_ITERS; ++i) {
			std::vector<int*> ptrs;
			ptrs.reserve(BULK_COUNT);

			for (std::size_t j = 0; j < BULK_COUNT; ++j) {
				int* p = stdAlloc.allocate(1);
				doNotOptimize(p);
				ptrs.push_back(p);
			}

			for (int* p : ptrs) stdAlloc.deallocate(p, 1);
		}
	});

	std::vector<std::string>              headers{ "Allocator", "Time (us)" };
	std::vector<std::vector<std::string>> data{
		{ "Stack", "malloc/free", "std::allocator" },
		{
			Table::format(stackTime.count(),  "us"),
			Table::format(mallocTime.count(), "us"),
			Table::format(stdTime.count(),    "us"),
		}
	};

	Table::table(
	    "Bulk Allocation  (" + std::to_string(BULK_ITERS) + " iters, "
	    + std::to_string(BULK_COUNT) + " allocs/iter)",
	    headers, data, 60);
}

// Mixed Allocation
// measures alternating Small, Medium, Large allocations vs malloc and std::allocator
void mixedAllocation() {
	const std::size_t stackSize = MIXED_ITERS * sizeof(Large);

	StackAllocator stack(stackSize);

	auto stackTime = duration([&] {
		for (std::size_t i = 0; i < MIXED_ITERS; ++i) {
			switch (i % 3) {
			case 0: {
				void* p = stack.allocate(sizeof(Small),  alignof(Small));
				doNotOptimize(static_cast<char*>(p));
				break;
			}
			case 1: {
				void* p = stack.allocate(sizeof(Medium), alignof(Medium));
				doNotOptimize(static_cast<char*>(p));
				break;
			}
			case 2: {
				void* p = stack.allocate(sizeof(Large),  alignof(Large));
				doNotOptimize(static_cast<char*>(p));
				break;
			}
			}
		}

		stack.reset();
	});

	auto mallocTime = duration([&] {
		std::vector<void*> ptrs;
		ptrs.reserve(MIXED_ITERS);

		for (std::size_t i = 0; i < MIXED_ITERS; ++i) {
			std::size_t sz = 0;
			switch (i % 3) {
			case 0:
				sz = sizeof(Small);
				break;
			case 1:
				sz = sizeof(Medium);
				break;
			case 2:
				sz = sizeof(Large);
				break;
			}
			void* p = std::malloc(sz);
			doNotOptimize(static_cast<char*>(p));
			ptrs.push_back(p);
		}

		for (void* p : ptrs) std::free(p);
	});

	std::allocator<std::byte> stdAlloc;
	auto stdTime = duration([&] {
		std::vector<std::byte*> ptrs;
		ptrs.reserve(MIXED_ITERS);

		for (std::size_t i = 0; i < MIXED_ITERS; ++i) {
			std::size_t sz = 0;
			switch (i % 3) {
			case 0:
				sz = sizeof(Small);
				break;
			case 1:
				sz = sizeof(Medium);
				break;
			case 2:
				sz = sizeof(Large);
				break;
			}
			std::byte* p = stdAlloc.allocate(sz);
			doNotOptimize(p);
			ptrs.push_back(p);
		}

		for (std::size_t i = 0; i < ptrs.size(); ++i) {
			std::size_t sz = 0;
			switch (i % 3) {
			case 0:
				sz = sizeof(Small);
				break;
			case 1:
				sz = sizeof(Medium);
				break;
			case 2:
				sz = sizeof(Large);
				break;
			}
			stdAlloc.deallocate(ptrs[i], sz);
		}
	});

	std::vector<std::string>              headers{ "Allocator", "Time (us)" };
	std::vector<std::vector<std::string>> data{
		{ "Stack", "malloc/free", "std::allocator" },
		{
			Table::format(stackTime.count(),  "us"),
			Table::format(mallocTime.count(), "us"),
			Table::format(stdTime.count(),    "us"),
		}
	};

	Table::table(
	    "Mixed Allocation  (" + std::to_string(MIXED_ITERS) + " iters, Small/Medium/Large)",
	    headers, data, 60);
}

// Marker Rewind
// measures partial stack rewind vs equivalent malloc/free of the same pattern
void markerRewind() {
	static constexpr std::size_t BASE_ALLOCS   = 16;
	static constexpr std::size_t REWIND_ALLOCS = 16;

	StackAllocator stack((BASE_ALLOCS + REWIND_ALLOCS) * sizeof(int));

	auto stackTime = duration([&] {
		for (std::size_t i = 0; i < MARKER_ITERS; ++i) {
			stack.reset();

			for (std::size_t j = 0; j < BASE_ALLOCS; ++j) {
				void* p = stack.allocate(sizeof(int), alignof(int));
				doNotOptimize(static_cast<int*>(p));
			}

			std::size_t marker = stack.getMarker();

			for (std::size_t j = 0; j < REWIND_ALLOCS; ++j) {
				void* p = stack.allocate(sizeof(int), alignof(int));
				doNotOptimize(static_cast<int*>(p));
			}

			stack.freeToMarker(marker);
		}
	});
	
	auto mallocTime = duration([&] {
		for (std::size_t i = 0; i < MARKER_ITERS; ++i) {
			std::vector<int*> base;
			base.reserve(BASE_ALLOCS);

			for (std::size_t j = 0; j < BASE_ALLOCS; ++j) {
				int* p = static_cast<int*>(std::malloc(sizeof(int)));
				doNotOptimize(p);
				base.push_back(p);
			}

			std::vector<int*> rewind;
			rewind.reserve(REWIND_ALLOCS);

			for (std::size_t j = 0; j < REWIND_ALLOCS; ++j) {
				int* p = static_cast<int*>(std::malloc(sizeof(int)));
				doNotOptimize(p);
				rewind.push_back(p);
			}

			for (int* p : rewind) std::free(p);
			for (int* p : base)   std::free(p);
		}
	});

	std::allocator<int> stdAlloc;
	auto stdTime = duration([&] {
		for (std::size_t i = 0; i < MARKER_ITERS; ++i) {
			std::vector<int*> base;
			base.reserve(BASE_ALLOCS);

			for (std::size_t j = 0; j < BASE_ALLOCS; ++j) {
				int* p = stdAlloc.allocate(1);
				doNotOptimize(p);
				base.push_back(p);
			}

			std::vector<int*> rewind;
			rewind.reserve(REWIND_ALLOCS);

			for (std::size_t j = 0; j < REWIND_ALLOCS; ++j) {
				int* p = stdAlloc.allocate(1);
				doNotOptimize(p);
				rewind.push_back(p);
			}

			for (int* p : rewind) stdAlloc.deallocate(p, 1);
			for (int* p : base)   stdAlloc.deallocate(p, 1);
		}
	});

	std::vector<std::string>              headers{ "Allocator", "Time (us)" };
	std::vector<std::vector<std::string>> data{
		{ "Stack", "malloc/free", "std::allocator" },
		{
			Table::format(stackTime.count(),  "us"),
			Table::format(mallocTime.count(), "us"),
			Table::format(stdTime.count(),    "us"),
		}
	};

	Table::table(
	    "Marker Rewind  (" + std::to_string(MARKER_ITERS) + " iters, "
	    + std::to_string(BASE_ALLOCS) + " base + " + std::to_string(REWIND_ALLOCS) + " rewind)",
	    headers, data, 60);
}

// Churn
// measures rapid allocate/reset cycles vs malloc and std::allocator
void churn() {
	static constexpr std::size_t STACK_SIZE = 64 * sizeof(int);

	StackAllocator stack(STACK_SIZE);

	auto stackTime = duration([&] {
		for (std::size_t i = 0; i < CHURN_ITERS; ++i) {
			void* p = stack.allocate(sizeof(int), alignof(int));
			doNotOptimize(static_cast<int*>(p));
			stack.reset();
		}
	});

	auto mallocTime = duration([&] {
		for (std::size_t i = 0; i < CHURN_ITERS; ++i) {
			int* p = static_cast<int*>(std::malloc(sizeof(int)));
			doNotOptimize(p);
			std::free(p);
		}
	});

	std::allocator<int> stdAlloc;
	auto stdTime = duration([&] {
		for (std::size_t i = 0; i < CHURN_ITERS; ++i) {
			int* p = stdAlloc.allocate(1);
			doNotOptimize(p);
			stdAlloc.deallocate(p, 1);
		}
	});

	std::vector<std::string>              headers{ "Allocator", "Time (us)" };
	std::vector<std::vector<std::string>> data{
		{ "Stack", "malloc/free", "std::allocator" },
		{
			Table::format(stackTime.count(),  "us"),
			Table::format(mallocTime.count(), "us"),
			Table::format(stdTime.count(),    "us"),
		}
	};

	Table::table(
	    "Churn  (" + std::to_string(CHURN_ITERS) + " alloc/reset cycles)",
	    headers, data, 44);
}

// Entry Point
int main() {
	singleAllocation();
	bulkAllocation();
	mixedAllocation();
	markerRewind();
	churn();

	return 0;
}

