Custom Allocators

Custom Allocators is a collection of memory allocators implemented in modern C++.

It was built to explore:

- custom memory management
- memory alignment
- object lifetime management
- allocation strategies
- memory fragmentation
- allocator performance

This project is primarily educational and is not intended to replace production-grade allocators.
The goal is to better understand how different allocation strategies work internally through custom implementations.

---

Implemented Allocators

ArenaAllocator

A linear allocator that allocates memory sequentially from a preallocated buffer.

Features

- Fast O(1) allocation
- Bulk reset with "clear()"
- Object creation and destruction
- Alignment support

---

PoolAllocator

A fixed-size allocator designed for objects of identical size.

Features

- O(1) allocation
- O(1) deallocation
- Free-list implementation
- Minimal fragmentation

---

StackAllocator

A Last-In-First-Out (LIFO) allocator.

Features

- O(1) allocation
- Marker rollback
- Scope rollback support
- Object creation and destruction
- Alignment support

---

FreeListAllocator

A variable-size allocator that reuses freed memory blocks.

Features

- Variable-size allocations
- Block splitting
- Block coalescing
- Object creation and destruction
- Alignment support

---

Project Structure

Custom Allocators/
├── ArenaAllocator/
│   ├── include/
│   ├── benchmarks/
│   ├── examples/
│   └── tests/
│
├── PoolAllocator/
│   ├── include/
│   ├── benchmarks/
│   ├── examples/
│   └── tests/
│
├── StackAllocator/
│   ├── include/
│   ├── benchmarks/
│   ├── examples/
│   └── tests/
│
└── FreeListAllocator/
    ├── include/
    ├── benchmarks/
    ├── examples/
    └── tests/

---

Build

Example

g++ -std=c++23 examples/example.cpp -Iinclude -o example
./example

Benchmark

g++ -std=c++23 benchmarks/benchmark.cpp -Iinclude -o benchmark
./benchmark

Test

g++ -std=c++23 tests/test.cpp -Iinclude -o test
./test

---

License

MIT License