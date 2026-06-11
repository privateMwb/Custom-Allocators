// ArenaAllocator Benchmark Suite
// Measures performance of arena allocation against traditional allocators:
//
// - single allocation (raw allocation overhead)
// - bulk allocation (high-throughput allocation)
// - mixed allocation sizes (real-world allocation patterns)
// - reset / frame cost (memory reuse efficiency)
//
// Benchmarks compare ArenaAllocator with malloc/free and std::allocator.

#include <iostream>
#include <cstddef>
#include <chrono>
#include <vector>
#include <string>
#include <memory>
#include <cstdlib>

#include "ArenaAllocator.h"
#include "ArenaScope.h"
#include "utils/Table.h"

// Config
static constexpr std::size_t SINGLE_ITERS = 100'000;
static constexpr std::size_t BULK_ITERS   = 1'000'000;
static constexpr std::size_t FRAME_ITERS  = 100'000;
static constexpr std::size_t MIXED_ITERS  = 100'000;

// Mixed-size payload types
struct Small  { char   data[4];  };
struct Medium { double data[4];  };
struct Large  { char   data[64]; };

// Optimization Sink
// Prevent the compiler from eliminating allocations
template<typename T>
inline void doNotOptimize(T* ptr) {
	asm volatile("" : : "r,m"(ptr) : "memory");
}

// Timing utility
// returns microseconds
template<typename F>
auto duration(F func) {
	auto start = std::chrono::steady_clock::now();
	func();
	auto end = std::chrono::steady_clock::now();

	return std::chrono::duration_cast<std::chrono::microseconds>(end - start);
}

// Single Allocation Speed
// One allocation per iteration — measures raw bump-pointer cost vs heap.
void singleAllocation() {
	ArenaAllocator arena(SINGLE_ITERS * (sizeof(int) + alignof(int)));

	auto arenaTime = duration([&] {
		for (std::size_t i = 0; i < SINGLE_ITERS; ++i) {
			int* p = arena.allocate<int>();
			doNotOptimize(p);
			arena.reset();
		}
	});

	auto mallocTime = duration([&] {
		for (std::size_t i = 0; i < SINGLE_ITERS; ++i) {
			int* p = static_cast<int*>(std::malloc(sizeof(int)));
			doNotOptimize(p);
			std::free(p);
		}
	});

	std::allocator<int> stdAlloc;
	auto stdTime = duration([&] {
		for (std::size_t i = 0; i < SINGLE_ITERS; ++i) {
			int* p = stdAlloc.allocate(1);
			doNotOptimize(p);
			stdAlloc.deallocate(p, 1);
		}
	});

	std::vector<std::string> headers{"Allocator", "Time (us)"};
	std::vector<std::vector<std::string>> data{
		{ "Arena", "malloc/free", "std::allocator" },
		{
			Table::format(arenaTime.count(),  "us"),
			Table::format(mallocTime.count(), "us"),
			Table::format(stdTime.count(),    "us"),
		}
	};

	Table::table(
	    "Single Allocation  (" + std::to_string(SINGLE_ITERS) + " iters)",
	    headers,data, 44);
}

// Bulk Allocations
// Allocate N objects in one pass — arena's strongest case.
void bulkAllocation() {
    const std::size_t arenaSize = BULK_ITERS * (sizeof(int) + alignof(int));

    ArenaAllocator arena(arenaSize);

    auto arenaTime = duration([&] {
        for (std::size_t i = 0; i < BULK_ITERS; ++i) {
            int* p = arena.allocate<int>();
            doNotOptimize(p);
        }
        arena.reset();
    });

    auto mallocTime = duration([&] {
        std::vector<int*> ptrs;
        ptrs.reserve(BULK_ITERS);

        for (std::size_t i = 0; i < BULK_ITERS; ++i) {
            int* p = static_cast<int*>(std::malloc(sizeof(int)));
            doNotOptimize(p);
            ptrs.push_back(p);
        }
        for (int* p : ptrs) std::free(p);
    });

    std::allocator<int> stdAlloc;
    auto stdTime = duration([&] {
        std::vector<int*> ptrs;
        ptrs.reserve(BULK_ITERS);

        for (std::size_t i = 0; i < BULK_ITERS; ++i) {
            int* p = stdAlloc.allocate(1);
            doNotOptimize(p);
            ptrs.push_back(p);
        }
        for (int* p : ptrs) stdAlloc.deallocate(p, 1);
    });

    std::vector<std::string> headers{ "Allocator", "Time (us)" };
    std::vector<std::vector<std::string>> data{
        { "Arena", "malloc/free", "std::allocator" },
        {
            Table::format(arenaTime.count(),  "us"),
            Table::format(mallocTime.count(), "us"),
            Table::format(stdTime.count(),    "us"),
        }
    };

    Table::table(
        "Bulk Allocation  (" + std::to_string(BULK_ITERS) + " iters)",
        headers, data, 44);
}

// Mixed Allocation Sizes
// Alternate Small/Medium/Large — simulates real usage patterns.
void mixedAllocation() {
    const std::size_t arenaSize = MIXED_ITERS * (sizeof(Large) + alignof(Large));

    ArenaAllocator arena(arenaSize);

    auto arenaTime = duration([&] {
        for (std::size_t i = 0; i < MIXED_ITERS; ++i) {
            switch (i % 3) {
                case 0: doNotOptimize(arena.allocate<Small>());  break;
                case 1: doNotOptimize(arena.allocate<Medium>()); break;
                case 2: doNotOptimize(arena.allocate<Large>());  break;
            }
        }
        arena.reset();
    });

    auto mallocTime = duration([&] {
        for (std::size_t i = 0; i < MIXED_ITERS; ++i) {
            void* p = nullptr;
            switch (i % 3) {
                case 0: p = std::malloc(sizeof(Small));  break;
                case 1: p = std::malloc(sizeof(Medium)); break;
                case 2: p = std::malloc(sizeof(Large));  break;
            }
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

    std::vector<std::string> headers{ "Allocator", "Time (us)" };
    std::vector<std::vector<std::string>> data{
        { "Arena", "malloc/free", "std::allocator" },
        {
            Table::format(arenaTime.count(),  "us"),
            Table::format(mallocTime.count(), "us"),
            Table::format(stdTime.count(),    "us"),
        }
    };

    Table::table(
        "Mixed Allocation  (" + std::to_string(MIXED_ITERS) + " iters, Small/Medium/Large)",
        headers, data, 60);
}


// Reset / Frame Cost
// Allocate N objects, reset, repeat — measures reuse cost vs heap.
void resetFrameCost() {
    static constexpr std::size_t OBJECTS_PER_FRAME = 64;
    const std::size_t arenaSize = OBJECTS_PER_FRAME * (sizeof(int) + alignof(int));

    ArenaAllocator arena(arenaSize);

    auto arenaResetTime = duration([&] {
        for (std::size_t i = 0; i < FRAME_ITERS; ++i) {
            for (std::size_t j = 0; j < OBJECTS_PER_FRAME; ++j) {
                int* p = arena.allocate<int>();
                doNotOptimize(p);
            }
            arena.reset();
        }
    });

    auto arenaFrameTime = duration([&] {
        for (std::size_t i = 0; i < FRAME_ITERS; ++i) {
            ArenaScope scope(arena);
            for (std::size_t j = 0; j < OBJECTS_PER_FRAME; ++j) {
                int* p = arena.allocate<int>();
                doNotOptimize(p);
            }
        }
    });

    auto mallocTime = duration([&] {
        for (std::size_t i = 0; i < FRAME_ITERS; ++i) {
            std::vector<int*> ptrs;
            ptrs.reserve(OBJECTS_PER_FRAME);

            for (std::size_t j = 0; j < OBJECTS_PER_FRAME; ++j) {
                int* p = static_cast<int*>(std::malloc(sizeof(int)));
                doNotOptimize(p);
                ptrs.push_back(p);
            }
            for (int* p : ptrs) std::free(p);
        }
    });

    std::vector<std::string> headers{ "Method", "Time (us)" };
    std::vector<std::vector<std::string>> data{
        { "Arena reset", "Arena frame", "malloc/free" },
        {
            Table::format(arenaResetTime.count(),  "us"),
            Table::format(arenaFrameTime.count(),  "us"),
            Table::format(mallocTime.count(),       "us"),
        }
    };

    Table::table(
        "Reset / Frame Cost  (" + std::to_string(FRAME_ITERS) + " iters, "
            + std::to_string(OBJECTS_PER_FRAME) + " objects/frame)",
        headers, data, 60);
}

// Entry Point
int main() {
	singleAllocation();
	bulkAllocation();
	mixedAllocation();
	resetFrameCost();

	return 0;
}

