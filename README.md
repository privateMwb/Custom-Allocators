# Custom Allocators

Custom Allocators is a collection of memory allocators written in modern C++23.

It was built to explore:
- manual memory management
- memory alignment
- object lifetime management
- allocation strategies
- fragmentation handling
- performance differences between allocators

This project is primarily educational and is not intended to replace standard allocators.
The goal is to understand how memory allocation works internally.

---

# Implemented Allocators

## ArenaAllocator

A linear allocator that allocates memory sequentially from a fixed buffer.

### Features
- Fast O(1) allocation
- Bulk reset (`clear`)
- Simple memory model
- Alignment support

---

## PoolAllocator

A fixed-size allocator for objects of identical size.

### Features
- O(1) allocation
- O(1) deallocation
- Free list of blocks
- No fragmentation (for fixed size objects)

---

## StackAllocator

A LIFO (last-in-first-out) allocator.

### Features
- O(1) allocation
- Marker-based rollback
- Scope-based memory control
- Alignment support

---

## FreeListAllocator

A general-purpose allocator using a free list of memory blocks.

### Features
- Variable size allocations
- Block splitting
- Block coalescing
- Alignment support
- Object lifecycle support

---

# Project Structure

```txt
CustomAllocators/
├── ArenaAllocator/
│   ├── include/
│   │   ├── ArenaAllocator.h
│   │   ├── ArenaAllocator.tpp
│   │   └── ArenaScope.h
│   │
│   ├── tests/
│   │   └── test.cpp
│   │
│   ├── examples/
│   │   └── examples.cpp
│   │
│   ├── benchmarks/
│   │   └── benchmarks.cpp
│   │
│   └── build/
│       ├── test
│       ├── examples
│       └── benchmarks
│
├── PoolAllocator/
│   ├── include/
│   │   ├── PoolAllocator.h
│   │   └── PoolAllocator.tpp
│   │
│   ├── tests/
│   │   └── test.cpp
│   │
│   ├── examples/
│   │   └── examples.cpp
│   │
│   ├── benchmarks/
│   │   └── benchmarks.cpp
│   │
│   └── build/
│       ├── test
│       ├── examples
│       └── benchmarks
│
├── StackAllocator/
│   ├── include/
│   │   ├── StackAllocator.h
│   │   ├── StackAllocator.tpp
│   │   └── StackScope.h
│   │
│   ├── tests/
│   │   └── test.cpp
│   │
│   ├── examples/
│   │   └── examples.cpp
│   │
│   ├── benchmarks/
│   │   └── benchmarks.cpp
│   │
│   └── build/
│       ├── test
│       ├── examples
│       └── benchmarks
│
├── FreeListAllocator/
│   ├── include/
│   │   ├── FreeListAllocator.h
│   │   └── FreeListAllocator.tpp
│   │
│   ├── tests/
│   │   └── test.cpp
│   │
│   ├── examples/
│   │   └── examples.cpp
│   │
│   ├── benchmarks/
│   │   └── benchmarks.cpp
│   │
│   └── build/
│       ├── test
│       ├── examples
│       └── benchmarks
│
├── README.md
└── LICENSE
```

---

# Build

## Example

```bash
g++ -std=c++23 examples/example.cpp -Iinclude -o example
./example
```

## Benchmark

```bash
g++ -std=c++23 benchmarks/benchmark.cpp -Iinclude -o benchmark
./benchmark
```

## Test

```bash
g++ -std=c++23 tests/test.cpp -Iinclude -o test
./test
```

---

# License

MIT License