// FreeListAllocator Benchmark Suite
// Measures performance of free-list allocation against traditional allocators:
//
// - single allocation (individual allocation cost)
// - bulk allocation (high-throughput allocation)
// - mixed allocation sizes (real-world allocation patterns)
// - fragmentation (coalesce and reuse after alternating deallocations)
// - churn (rapid allocate/deallocate cycles)
//
// Benchmarks compare FreeListAllocator with malloc/free and std::allocator.

#include <iostream>
#include <cstddef>
#include <chrono>
#include <memory>
#include <vector>

#include "FreeListAllocator.h"
#include "utils/Table.h"

// Config
static constexpr std::size_t SINGLE_ITERS = 100'000;
static constexpr std::size_t BULK_COUNT   = 1'000;
static constexpr std::size_t BULK_ITERS   = 1'000;
static constexpr std::size_t MIXED_ITERS  = 100'000;
static constexpr std::size_t FRAG_ITERS   = 10'000;
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
// measures pure allocation speed by allocating and deallocating one block at a time
void singleAllocation() {
	static constexpr std::size_t BLOCK_SIZE = sizeof(int) + sizeof(std::size_t) * 2;

	FreeListAllocator allocator(BLOCK_SIZE * 64);

	auto freeListTime = duration([&] {
		for (std::size_t i = 0; i < SINGLE_ITERS; ++i) {
			void* p = allocator.allocate(sizeof(int), alignof(int));
			doNotOptimize(static_cast<int*>(p));
			allocator.deallocate(p);
		}
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
		{ "FreeList", "malloc/free", "std::allocator" },
		{
			Table::format(freeListTime.count(), "us"),
			Table::format(mallocTime.count(),   "us"),
			Table::format(stdTime.count(),      "us"),
		}
	};

	Table::table(
	    "Single Allocation  (" + std::to_string(SINGLE_ITERS) + " iters)",
	    headers, data, 44);
}

// Bulk Allocation
// measures repeated alloc/dealloc cycles vs malloc and std::allocator
void bulkAllocation() {
    static constexpr std::size_t BLOCK_SIZE = sizeof(int) + sizeof(std::size_t) * 2;

    FreeListAllocator allocator(BLOCK_SIZE * BULK_COUNT);

    auto freeListTime = duration([&] {
        for (std::size_t i = 0; i < BULK_ITERS; ++i) {
            for (std::size_t j = 0; j < BULK_COUNT; ++j) {
                void* p = allocator.allocate(sizeof(int), alignof(int));
                doNotOptimize(static_cast<int*>(p));
                allocator.deallocate(p);
            }
        }
    });

    auto mallocTime = duration([&] {
        for (std::size_t i = 0; i < BULK_ITERS; ++i) {
            for (std::size_t j = 0; j < BULK_COUNT; ++j) {
                int* p = static_cast<int*>(std::malloc(sizeof(int)));
                doNotOptimize(p);
                std::free(p);
            }
        }
    });

    std::allocator<int> stdAlloc;
    auto stdTime = duration([&] {
        for (std::size_t i = 0; i < BULK_ITERS; ++i) {
            for (std::size_t j = 0; j < BULK_COUNT; ++j) {
                int* p = stdAlloc.allocate(1);
                doNotOptimize(p);
                stdAlloc.deallocate(p, 1);
            }
        }
    });

    std::vector<std::string>              headers{ "Allocator", "Time (us)" };
    std::vector<std::vector<std::string>> data{
        { "FreeList", "malloc/free", "std::allocator" },
        {
            Table::format(freeListTime.count(), "us"),
            Table::format(mallocTime.count(),   "us"),
            Table::format(stdTime.count(),      "us"),
        }
    };

    Table::table(
        "Bulk Allocation  (" + std::to_string(BULK_ITERS) + " iters, "
            + std::to_string(BULK_COUNT) + " allocs/iter)",
        headers, data, 60);
}

// Mixed Allocation
// measures alternating Small, Medium, Large alloc/dealloc cycles vs malloc and std::allocator
void mixedAllocation() {
    static constexpr std::size_t BLOCK_SIZE = sizeof(Large) + sizeof(std::size_t) * 2;

    FreeListAllocator allocator(BLOCK_SIZE * 64);

    auto freeListTime = duration([&] {
        for (std::size_t i = 0; i < MIXED_ITERS; ++i) {
            switch (i % 3) {
                case 0: { void* p = allocator.allocate(sizeof(Small),  alignof(Small));  doNotOptimize(static_cast<char*>(p)); allocator.deallocate(p); break; }
                case 1: { void* p = allocator.allocate(sizeof(Medium), alignof(Medium)); doNotOptimize(static_cast<char*>(p)); allocator.deallocate(p); break; }
                case 2: { void* p = allocator.allocate(sizeof(Large),  alignof(Large));  doNotOptimize(static_cast<char*>(p)); allocator.deallocate(p); break; }
            }
        }
    });

    auto mallocTime = duration([&] {
        for (std::size_t i = 0; i < MIXED_ITERS; ++i) {
            std::size_t sz = 0;
            switch (i % 3) {
                case 0: sz = sizeof(Small);  break;
                case 1: sz = sizeof(Medium); break;
                case 2: sz = sizeof(Large);  break;
            }
            void* p = std::malloc(sz);
            doNotOptimize(static_cast<char*>(p));
            std::free(p);
        }
    });

    std::allocator<std::byte> stdAlloc;
    auto stdTime = duration([&] {
        for (std::size_t i = 0; i < MIXED_ITERS; ++i) {
            std::size_t sz = 0;
            switch (i % 3) {
                case 0: sz = sizeof(Small);  break;
                case 1: sz = sizeof(Medium); break;
                case 2: sz = sizeof(Large);  break;
            }
            std::byte* p = stdAlloc.allocate(sz);
            doNotOptimize(p);
            stdAlloc.deallocate(p, sz);
        }
    });

    std::vector<std::string>              headers{ "Allocator", "Time (us)" };
    std::vector<std::vector<std::string>> data{
        { "FreeList", "malloc/free", "std::allocator" },
        {
            Table::format(freeListTime.count(), "us"),
            Table::format(mallocTime.count(),   "us"),
            Table::format(stdTime.count(),      "us"),
        }
    };

    Table::table(
        "Mixed Allocation  (" + std::to_string(MIXED_ITERS) + " iters, Small/Medium/Large)",
        headers, data, 60);
}

// Fragmentation
// measures coalesce and reuse performance after alternating deallocations
void fragmentation() {
	static constexpr std::size_t BLOCK_COUNT = 8;
	static constexpr std::size_t BLOCK_SIZE  = sizeof(int) + sizeof(std::size_t) * 2;

	FreeListAllocator allocator(BLOCK_SIZE * BLOCK_COUNT);

	auto freeListTime = duration([&] {
		for (std::size_t i = 0; i < FRAG_ITERS; ++i) {
			std::vector<void*> ptrs;
			ptrs.reserve(BLOCK_COUNT);

			for (std::size_t j = 0; j < BLOCK_COUNT; ++j) {
				void* p = allocator.allocate(sizeof(int), alignof(int));
				doNotOptimize(static_cast<int*>(p));
				ptrs.push_back(p);
			}

			// Free alternating blocks — coalesce triggered on next allocate() when no block fits
			for (std::size_t j = 0; j < BLOCK_COUNT; j += 2)
				allocator.deallocate(ptrs[j]);

			for (std::size_t j = 1; j < BLOCK_COUNT; j += 2)
				allocator.deallocate(ptrs[j]);
		}
	});

	auto mallocTime = duration([&] {
		for (std::size_t i = 0; i < FRAG_ITERS; ++i) {
			std::vector<int*> ptrs;
			ptrs.reserve(BLOCK_COUNT);

			for (std::size_t j = 0; j < BLOCK_COUNT; ++j) {
				int* p = static_cast<int*>(std::malloc(sizeof(int)));
				doNotOptimize(p);
				ptrs.push_back(p);
			}

			for (std::size_t j = 0; j < BLOCK_COUNT; j += 2) std::free(ptrs[j]);
			for (std::size_t j = 1; j < BLOCK_COUNT; j += 2) std::free(ptrs[j]);
		}
	});

	std::allocator<int> stdAlloc;
	auto stdTime = duration([&] {
		for (std::size_t i = 0; i < FRAG_ITERS; ++i) {
			std::vector<int*> ptrs;
			ptrs.reserve(BLOCK_COUNT);

			for (std::size_t j = 0; j < BLOCK_COUNT; ++j) {
				int* p = stdAlloc.allocate(1);
				doNotOptimize(p);
				ptrs.push_back(p);
			}

			for (std::size_t j = 0; j < BLOCK_COUNT; j += 2) stdAlloc.deallocate(ptrs[j], 1);
			for (std::size_t j = 1; j < BLOCK_COUNT; j += 2) stdAlloc.deallocate(ptrs[j], 1);
		}
	});

	std::vector<std::string>              headers{ "Allocator", "Time (us)" };
	std::vector<std::vector<std::string>> data{
		{ "FreeList", "malloc/free", "std::allocator" },
		{
			Table::format(freeListTime.count(), "us"),
			Table::format(mallocTime.count(),   "us"),
			Table::format(stdTime.count(),      "us"),
		}
	};

	Table::table(
	    "Fragmentation  (" + std::to_string(FRAG_ITERS) + " iters, "
	    + std::to_string(BLOCK_COUNT) + " blocks/iter)",
	    headers, data, 60);
}

// Churn
// measures rapid individual allocate/deallocate cycles vs malloc and std::allocator
void churn() {
	static constexpr std::size_t BLOCK_SIZE = sizeof(int) + sizeof(std::size_t) * 2;

	FreeListAllocator allocator(BLOCK_SIZE * 64);

	auto freeListTime = duration([&] {
		for (std::size_t i = 0; i < CHURN_ITERS; ++i) {
			void* p = allocator.allocate(sizeof(int), alignof(int));
			doNotOptimize(static_cast<int*>(p));
			allocator.deallocate(p);
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
		{ "FreeList", "malloc/free", "std::allocator" },
		{
			Table::format(freeListTime.count(), "us"),
			Table::format(mallocTime.count(),   "us"),
			Table::format(stdTime.count(),      "us"),
		}
	};

	Table::table(
	    "Churn  (" + std::to_string(CHURN_ITERS) + " alloc/deallocate cycles)",
	    headers, data, 44);
}

// Entry Point
int main() {
	singleAllocation();
	bulkAllocation();
	mixedAllocation();
	fragmentation();
	churn();

	return 0;
}

