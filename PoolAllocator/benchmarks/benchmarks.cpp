// PoolAllocator Benchmark Suite
// Measures performance of fixed-size pool allocation against traditional allocators:
//
// - single allocation (individual allocation cost)
// - bulk allocation (high-throughput allocation)
// - mixed allocation sizes (real-world allocation patterns)
// - churn (rapid allocate/deallocate cycles)
//
// Benchmarks compare PoolAllocator with malloc/free and std::allocator.

#include <iostream>
#include <cstddef>
#include <chrono>
#include <memory>
#include <vector>

#include "PoolAllocator.h"
#include "utils/Table.h"

// Config
static constexpr std::size_t SINGLE_ITERS = 100'000;
static constexpr std::size_t BULK_COUNT   = 1'000;
static constexpr std::size_t BULK_ITERS   = 1'000;
static constexpr std::size_t MIXED_ITERS  = 100'000;
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
struct Small  { char data[4];  };
struct Medium { char data[16]; };
struct Large  { char data[64]; };

// Single Allocation
// measures pure allocation speed by filling the pool completely before draining it
void singleAllocation() {
    PoolAllocator pool(sizeof(int), SINGLE_ITERS);

    auto poolTime = duration([&] {
        std::vector<void*> ptrs;
        ptrs.reserve(SINGLE_ITERS);

        for (std::size_t i = 0; i < SINGLE_ITERS; ++i) {
            void* p = pool.allocate();
            doNotOptimize(static_cast<int*>(p));
            ptrs.push_back(p);
        }

        for (void* p : ptrs) pool.deallocate(p);
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
        { "Pool", "malloc/free", "std::allocator" },
        {
            Table::format(poolTime.count(),   "us"),
            Table::format(mallocTime.count(), "us"),
            Table::format(stdTime.count(),    "us"),
        }
    };

    Table::table(
        "Single Allocation  (" + std::to_string(SINGLE_ITERS) + " iters)",
        headers, data, 44);
}

// Bulk Allocation
// measures filling and draining the entire pool repeatedly vs malloc and std::allocator
void bulkAllocation() {
    PoolAllocator pool(sizeof(int), BULK_COUNT);

    auto poolTime = duration([&] {
        for (std::size_t i = 0; i < BULK_ITERS; ++i) {
            std::vector<void*> ptrs;
            ptrs.reserve(BULK_COUNT);

            for (std::size_t j = 0; j < BULK_COUNT; ++j) {
                void* p = pool.allocate();
                doNotOptimize(static_cast<int*>(p));
                ptrs.push_back(p);
            }

            for (void* p : ptrs) pool.deallocate(p);
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
        { "Pool", "malloc/free", "std::allocator" },
        {
            Table::format(poolTime.count(),   "us"),
            Table::format(mallocTime.count(), "us"),
            Table::format(stdTime.count(),    "us"),
        }
    };

    Table::table(
        "Bulk Allocation  (" + std::to_string(BULK_ITERS) + " iters, "
            + std::to_string(BULK_COUNT) + " blocks/iter)",
        headers, data, 60);
}

// Mixed Allocation
// measures alternating Small, Medium, Large allocations vs malloc and std::allocator
void mixedAllocation() {
    const std::size_t blockSize = sizeof(Large);

    PoolAllocator pool(blockSize, MIXED_ITERS);

    auto poolTime = duration([&] {
        std::vector<void*> ptrs;
        ptrs.reserve(MIXED_ITERS);

        for (std::size_t i = 0; i < MIXED_ITERS; ++i) {
            void* p = pool.allocate();
            doNotOptimize(static_cast<char*>(p));
            ptrs.push_back(p);
        }

        for (void* p : ptrs) pool.deallocate(p);
    });

    auto mallocTime = duration([&] {
        std::vector<void*> ptrs;
        ptrs.reserve(MIXED_ITERS);

        for (std::size_t i = 0; i < MIXED_ITERS; ++i) {
            std::size_t sz = 0;
            switch (i % 3) {
                case 0: sz = sizeof(Small);  break;
                case 1: sz = sizeof(Medium); break;
                case 2: sz = sizeof(Large);  break;
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
                case 0: sz = sizeof(Small);  break;
                case 1: sz = sizeof(Medium); break;
                case 2: sz = sizeof(Large);  break;
            }
            std::byte* p = stdAlloc.allocate(sz);
            doNotOptimize(p);
            ptrs.push_back(p);
        }

        for (std::size_t i = 0; i < ptrs.size(); ++i) {
            std::size_t sz = 0;
            switch (i % 3) {
                case 0: sz = sizeof(Small);  break;
                case 1: sz = sizeof(Medium); break;
                case 2: sz = sizeof(Large);  break;
            }
            stdAlloc.deallocate(ptrs[i], sz);
        }
    });

    std::vector<std::string>              headers{ "Allocator", "Time (us)" };
    std::vector<std::vector<std::string>> data{
        { "Pool", "malloc/free", "std::allocator" },
        {
            Table::format(poolTime.count(),   "us"),
            Table::format(mallocTime.count(), "us"),
            Table::format(stdTime.count(),    "us"),
        }
    };

    Table::table(
        "Mixed Allocation  (" + std::to_string(MIXED_ITERS) + " iters, Small/Medium/Large)",
        headers, data, 60);
}

// Churn
// measures rapid individual allocate/deallocate cycles vs malloc and std::allocator
void churn() {
    static constexpr std::size_t POOL_SIZE = 64;

    PoolAllocator pool(sizeof(int), POOL_SIZE);

    auto poolTime = duration([&] {
        for (std::size_t i = 0; i < CHURN_ITERS; ++i) {
            void* p = pool.allocate();
            doNotOptimize(static_cast<int*>(p));
            pool.deallocate(p);
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
        { "Pool", "malloc/free", "std::allocator" },
        {
            Table::format(poolTime.count(),   "us"),
            Table::format(mallocTime.count(), "us"),
            Table::format(stdTime.count(),    "us"),
        }
    };

    Table::table(
        "Churn  (" + std::to_string(CHURN_ITERS) + " alloc/dealloc cycles)",
        headers, data, 44);
}

// Entry Point
int main() {
    singleAllocation();
    bulkAllocation();
    mixedAllocation();
    churn();

    return 0;
}


