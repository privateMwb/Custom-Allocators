# Custom Allocators

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue)](https://en.cppreference.com/w/cpp/23)
[![Status](https://img.shields.io/badge/status-learning%20project-green)](https://github.com/privateMwb/Custom-Allocators)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

A collection of four custom memory allocators implemented from scratch in **C++23**, built to explore manual memory management, alignment, allocation strategies, fragmentation handling, and real performance tradeoffs against `malloc` and `std::allocator`.

---

## Table of Contents

- [Overview](#overview)
- [Motivation](#motivation)
- [Allocators](#allocators)
  - [ArenaAllocator](#arenaallocator)
  - [PoolAllocator](#poolallocator)
  - [StackAllocator](#stackallocator)
  - [FreeListAllocator](#freelistallocator)
- [Allocator Comparison](#allocator-comparison)
- [Project Structure](#project-structure)
- [Benchmark Results](#benchmark-results)
  - [ArenaAllocator Benchmarks](#arenaallocator-benchmarks)
  - [PoolAllocator Benchmarks](#poolallocator-benchmarks)
  - [StackAllocator Benchmarks](#stackallocator-benchmarks)
  - [FreeListAllocator Benchmarks](#freelistallocator-benchmarks)
  - [Overall Summary](#overall-summary)
- [Build Instructions](#build-instructions)
- [Notes](#notes)
- [License](#license)

---

## Overview

This project implements four allocators that each target a different memory usage pattern. Rather than relying on `new`/`delete` or `malloc`/`free`, each allocator manages a pre-allocated memory block and serves requests from it — avoiding the overhead, fragmentation, and unpredictability of the general-purpose heap.

Every allocator includes:
- A header-only C++23 implementation (`include/`)
- Unit tests (`tests/`)
- Usage examples (`examples/`)
- A benchmark suite comparing against `malloc/free` and `std::allocator` (`benchmarks/`)

---

## Motivation

This project was built to deeply understand:

- Manual memory management and object lifetime control
- Memory alignment and padding
- How different allocation patterns affect performance
- Fragmentation: what causes it and how to fight it
- Why general-purpose allocators are slow in specialized workloads
- The design tradeoffs between speed, flexibility, and complexity

---

## Allocators

### ArenaAllocator

A **linear (bump-pointer) allocator** that allocates sequentially from a fixed-size buffer. Individual deallocations are not supported — the entire arena is reset at once.

**How it works:**

```
buffer
  ↓
[ used | used | used | used |     free     ]
                             ↑
                           offset
```

Each allocation simply advances the offset by the requested (aligned) size. Reset sets the offset back to zero — an O(1) operation regardless of how many objects were allocated.

**Extras:** `ArenaScope` provides RAII-style scoped reset — when the scope exits, the arena rewinds to its state at scope entry.

**Best for:** Frame allocators, per-request scratch buffers, temporary workloads with well-defined lifetimes.

**Features:**
- O(1) allocation
- O(1) bulk reset (`clear`)
- Scoped lifetime support via `ArenaScope`
- Alignment support

---

### PoolAllocator

A **fixed-size block allocator** that manages a pool of equally-sized chunks. A free list tracks available blocks; allocation and deallocation are both O(1).

**How it works:**

```
buffer
  ↓
[ block ][ block ][ block ][ block ][ block ]
    ↑         ↑
free_list  (linked via embedded next pointers)
```

Each free block stores a pointer to the next free block inside its own memory. On allocation, the head of the free list is returned and the list advances. On deallocation, the block is prepended back to the list.

**Best for:** Object pools (bullets, particles, entities), any scenario with repeated alloc/dealloc of same-size objects.

**Features:**
- O(1) allocation
- O(1) deallocation
- Zero fragmentation for fixed-size objects
- Free list managed via embedded pointers

---

### StackAllocator

A **LIFO allocator** that grows from one end of a buffer. Each allocation stores a small header so it can be individually unwound. A marker system allows rewinding to any saved point in the stack.

**How it works:**

```
buffer
  ↓
[ header | data | header | data | header | data |   free   ]
                                                  ↑
                                               top (offset)
```

Deallocation must follow LIFO order — you can only free the most recently allocated block, or rewind to a saved marker.

**Extras:** `StackScope` provides RAII-style marker save/restore — on scope exit, the stack rewinds to its state at scope entry.

**Best for:** Level/scene loading, function call frame simulation, any workload with strict LIFO memory usage.

**Features:**
- O(1) allocation
- O(1) deallocation (LIFO only)
- Marker-based rollback (`saveMarker`, `rewindTo`)
- Scoped lifetime support via `StackScope`
- Alignment support

---

### FreeListAllocator

A **general-purpose allocator** that supports variable-size allocations and deallocations in any order. A linked list tracks free blocks; on deallocation, adjacent free blocks are **coalesced** to reduce fragmentation.

**How it works:**

```
buffer
  ↓
[ header | used | header |   free   | header | used | header |  free  ]
                               ↑                                  ↑
                           free_list ────────────────────────►  next
```

Each block carries a header with its size and status. On allocation, the free list is searched for a suitable block (first-fit), which may be **split** if significantly larger than needed. On deallocation, neighboring free blocks are merged back together.

**Best for:** General dynamic allocation where sizes vary and full alloc/dealloc flexibility is needed without relying on `malloc`.

**Features:**
- Variable-size allocations
- Block splitting (avoids wasting large blocks on small requests)
- Block coalescing (merges adjacent free blocks to reduce fragmentation)
- Full object lifecycle support (`construct` / `destroy`)
- Alignment support

---

## Allocator Comparison

| Feature                  | Arena | Pool  | Stack    | FreeList |
|--------------------------|:-----:|:-----:|:--------:|:--------:|
| Allocation complexity    | O(1)  | O(1)  | O(1)     | O(n)     |
| Deallocation complexity  | N/A ¹ | O(1)  | O(1) ²   | O(n)     |
| Variable-size allocs     | ✅    | ❌    | ✅       | ✅       |
| Individual deallocation  | ❌    | ✅    | LIFO only| ✅       |
| Fragmentation            | None  | None  | None     | Low ³    |
| Scoped lifetime support  | ✅    | ❌    | ✅       | ❌       |
| Block coalescing         | ❌    | ❌    | ❌       | ✅       |
| General purpose          | ❌    | ❌    | ❌       | ✅       |

> ¹ Arena only supports bulk reset, not individual frees.  
> ² Stack deallocation must follow LIFO order.  
> ³ FreeList coalescing keeps fragmentation low but not zero.

---

## Project Structure

```
Custom-Allocators/
├── ArenaAllocator/
│   ├── include/
│   │   ├── ArenaAllocator.h
│   │   ├── ArenaAllocator.tpp
│   │   └── ArenaScope.h
│   ├── tests/
│   │   └── test.cpp
│   ├── examples/
│   │   └── examples.cpp
│   ├── benchmarks/
│   │   └── benchmarks.cpp
│   └── build/
│
├── PoolAllocator/
│   ├── include/
│   │   ├── PoolAllocator.h
│   │   └── PoolAllocator.tpp
│   ├── tests/
│   │   └── test.cpp
│   ├── examples/
│   │   └── examples.cpp
│   ├── benchmarks/
│   │   └── benchmarks.cpp
│   └── build/
│
├── StackAllocator/
│   ├── include/
│   │   ├── StackAllocator.h
│   │   ├── StackAllocator.tpp
│   │   └── StackScope.h
│   ├── tests/
│   │   └── test.cpp
│   ├── examples/
│   │   └── examples.cpp
│   ├── benchmarks/
│   │   └── benchmarks.cpp
│   └── build/
│
├── FreeListAllocator/
│   ├── include/
│   │   ├── FreeListAllocator.h
│   │   └── FreeListAllocator.tpp
│   ├── tests/
│   │   └── test.cpp
│   ├── examples/
│   │   └── examples.cpp
│   ├── benchmarks/
│   │   └── benchmarks.cpp
│   └── build/
│
├── README.md
└── LICENSE
```

---

## Benchmark Results

All benchmarks compare each allocator against `malloc/free` and `std::allocator`. Results were collected without `-O2`/`-O3` — use optimized builds for production comparisons.

---

### ArenaAllocator Benchmarks

#### Single Allocation — 100,000 iterations

| Allocator      | Time (us) | vs malloc       |
|----------------|----------:|----------------:|
| Arena          | 54        | **224× faster** |
| malloc/free    | 12,127    | baseline        |
| std::allocator | 37,065    | —               |

#### Bulk Allocation — 1,000,000 iterations

| Allocator      | Time (us) | vs malloc      |
|----------------|----------:|---------------:|
| Arena          | 3,726     | **66× faster** |
| malloc/free    | 246,024   | baseline       |
| std::allocator | 232,590   | —              |

#### Mixed Allocation — 100,000 iterations (Small / Medium / Large)

| Allocator      | Time (us) | vs malloc      |
|----------------|----------:|---------------:|
| Arena          | 523       | **68× faster** |
| malloc/free    | 35,770    | baseline       |
| std::allocator | 22,323    | —              |

#### Reset / Frame Cost — 100,000 iterations, 64 objects/frame

| Method         | Time (us) | vs malloc      |
|----------------|----------:|---------------:|
| Arena reset    | 25,769    | **36× faster** |
| Arena frame    | 29,635    | **31× faster** |
| malloc/free    | 939,982   | baseline       |

**Takeaway:** Arena is the fastest allocator across all workloads — up to 224× faster than `malloc` for single allocations. The bulk reset model makes it ideal for frame-based workloads where everything is discarded at end-of-frame.

---

### PoolAllocator Benchmarks

#### Single Allocation — 100,000 iterations

| Allocator      | Time (us) | vs malloc     |
|----------------|----------:|--------------:|
| Pool           | 2,847     | **8× faster** |
| malloc/free    | 23,153    | baseline      |
| std::allocator | 21,040    | —             |

#### Bulk Allocation — 1,000 iterations × 1,000 blocks

| Allocator      | Time (us) | vs malloc      |
|----------------|----------:|---------------:|
| Pool           | 5,427     | **27× faster** |
| malloc/free    | 151,524   | baseline       |
| std::allocator | 155,559   | —              |

#### Mixed Allocation — 100,000 iterations (Small / Medium / Large)

| Allocator      | Time (us) | vs malloc     |
|----------------|----------:|--------------:|
| Pool           | 3,953     | **7× faster** |
| malloc/free    | 30,160    | baseline      |
| std::allocator | 31,879    | —             |

#### Churn — 100,000 alloc/dealloc cycles

| Allocator      | Time (us) | vs malloc       |
|----------------|----------:|----------------:|
| Pool           | 270       | **128× faster** |
| malloc/free    | 34,759    | baseline        |
| std::allocator | 18,871    | —               |

**Takeaway:** Pool's O(1) deallocation via free list makes it exceptional under churn — 128× faster than `malloc` when objects are constantly allocated and freed. The ideal fit for object reuse patterns like bullets, particles, or pooled connections.

---

### StackAllocator Benchmarks

#### Single Allocation — 100,000 iterations

| Allocator      | Time (us) | vs malloc      |
|----------------|----------:|---------------:|
| Stack          | 369       | **66× faster** |
| malloc/free    | 24,559    | baseline       |
| std::allocator | 21,082    | —              |

#### Bulk Allocation — 1,000 iterations × 1,000 allocs

| Allocator      | Time (us) | vs malloc      |
|----------------|----------:|---------------:|
| Stack          | 3,699     | **41× faster** |
| malloc/free    | 152,575   | baseline       |
| std::allocator | 156,146   | —              |

#### Mixed Allocation — 100,000 iterations (Small / Medium / Large)

| Allocator      | Time (us) | vs malloc      |
|----------------|----------:|---------------:|
| Stack          | 487       | **59× faster** |
| malloc/free    | 29,082    | baseline       |
| std::allocator | 34,244    | —              |

#### Marker Rewind — 100,000 iterations, 16 base + 16 rewind allocs

| Allocator      | Time (us) | vs malloc      |
|----------------|----------:|---------------:|
| Stack          | 11,916    | **38× faster** |
| malloc/free    | 463,143   | baseline       |
| std::allocator | 475,740   | —              |

#### Churn — 100,000 alloc/reset cycles

| Allocator      | Time (us) | vs malloc       |
|----------------|----------:|----------------:|
| Stack          | 108       | **111× faster** |
| malloc/free    | 12,001    | baseline        |
| std::allocator | 12,388    | —               |

**Takeaway:** Stack is consistently fast across all workloads. The marker rewind test highlights its biggest strength — 38× faster than `malloc` for scope-based memory patterns like level loading or function call frames. Churn performance (111×) rivals the Pool allocator.

---

### FreeListAllocator Benchmarks

#### Single Allocation — 100,000 iterations

| Allocator      | Time (us) | vs malloc      |
|----------------|----------:|---------------:|
| FreeList       | 1,618     | **15× faster** |
| malloc/free    | 24,457    | baseline       |
| std::allocator | 21,138    | —              |

#### Bulk Allocation — 1,000 iterations × 1,000 allocs

| Allocator      | Time (us) | vs malloc     |
|----------------|----------:|--------------:|
| FreeList       | 15,471    | **7× faster** |
| malloc/free    | 120,943   | baseline      |
| std::allocator | 123,788   | —             |

#### Mixed Allocation — 100,000 iterations (Small / Medium / Large)

| Allocator      | Time (us) | vs malloc     |
|----------------|----------:|--------------:|
| FreeList       | 2,574     | **4× faster** |
| malloc/free    | 12,173    | baseline      |
| std::allocator | 12,545    | —             |

#### Fragmentation — 10,000 iterations, 8 blocks/iter

| Allocator      | Time (us) | vs malloc     |
|----------------|----------:|--------------:|
| FreeList       | 3,072     | **3× faster** |
| malloc/free    | 11,365    | baseline      |
| std::allocator | 11,862    | —             |

#### Churn — 100,000 alloc/dealloc cycles

| Allocator      | Time (us) | vs malloc     |
|----------------|----------:|--------------:|
| FreeList       | 1,546     | **7× faster** |
| malloc/free    | 12,084    | baseline      |
| std::allocator | 12,422    | —             |

**Takeaway:** FreeList is the most flexible allocator — variable sizes, arbitrary deallocation order — while still outperforming `malloc` across every test. The fragmentation benchmark confirms that block coalescing keeps it competitive even under stress patterns that would degrade a naive free list.

---

### Overall Summary

| Allocator | Best Use Case               | Standout Benchmark    | Peak Speedup vs malloc |
|-----------|-----------------------------|-----------------------|------------------------|
| Arena     | Frame / scratch buffers     | Single allocation     | ~224×                  |
| Pool      | Fixed-size object reuse     | Churn (alloc/dealloc) | ~128×                  |
| Stack     | Scoped / LIFO workloads     | Churn (alloc/reset)   | ~111×                  |
| FreeList  | General dynamic allocation  | Single allocation     | ~15×                   |

> The more constraints an allocator imposes on usage patterns, the faster it becomes. Arena (no individual frees) is the fastest; FreeList (fully flexible) is the most general but slowest of the four — yet still significantly beats `malloc` across every workload.

---

## Build Instructions

### Requirements

- C++23-compatible compiler: GCC 13+, Clang 17+, or MSVC 19.38+
- No external dependencies — each allocator is header-only

Each allocator is self-contained under its own directory. Navigate into the one you want, then:

### Compile & Run Tests

```bash
g++ -std=c++23 tests/test.cpp -Iinclude -o build/test
./build/test
```

### Compile & Run Examples

```bash
g++ -std=c++23 examples/examples.cpp -Iinclude -o build/examples
./build/examples
```

### Compile & Run Benchmarks

```bash
g++ -std=c++23 benchmarks/benchmarks.cpp -Iinclude -o build/benchmarks
./build/benchmarks
```

> Use `-O2` or `-O3` for meaningful benchmark results. Debug builds without optimizations will significantly distort timing.

---

## Notes

- **Not production-ready.** This is an educational project — use battle-tested allocators in real codebases.
- Thread safety is not implemented. All allocators are single-threaded.
- Benchmark results were collected on a specific machine and compiler configuration — your numbers may vary.
- FreeList's O(n) allocation cost comes from the free-list search (first-fit). In practice this is fast when fragmentation is low, but degrades under adversarial allocation patterns.
- Arena and Stack do not call destructors on reset — objects must be trivially destructible or manually destroyed before resetting.

---

## License

[MIT](LICENSE) — free to use, modify, and distribute for educational and personal purposes.
