Custom Allocators

A collection of custom memory allocators implemented in modern C++23 for learning and experimentation with memory management techniques.

Implemented Allocators

Arena Allocator

A linear allocator that allocates memory sequentially from a preallocated buffer.

Features

- Fast O(1) allocation
- Bulk reset with "clear()"
- Object construction and destruction
- Alignment support

Tradeoffs

- Individual deallocation is not supported
- Entire arena must be reset at once

---

Pool Allocator

A fixed-size allocator that manages memory blocks of identical size.

Features

- O(1) allocation
- O(1) deallocation
- Free-list based implementation
- Minimal fragmentation

Tradeoffs

- All allocations must fit the configured block size

---

Stack Allocator

A Last-In-First-Out (LIFO) allocator.

Features

- O(1) allocation
- O(1) rollback using markers
- Scope-based rollback support
- Alignment support

Tradeoffs

- Memory must be released in reverse allocation order

---

Free List Allocator

A variable-size allocator that reuses freed blocks.

Features

- Variable-size allocations
- Block splitting
- Block coalescing
- Object construction and destruction
- Alignment support

Tradeoffs

- Allocation is slower than Arena, Pool, and Stack allocators
- Fragmentation can occur without coalescing

---

Project Structure

Custom Allocators/
│
├── ArenaAllocator/
│   ├── include/
│   ├── tests/
│   ├── benchmarks/
│   └── examples/
│
├── PoolAllocator/
│   ├── include/
│   ├── tests/
│   ├── benchmarks/
│   └── examples/
│
├── StackAllocator/
│   ├── include/
│   ├── tests/
│   ├── benchmarks/
│   └── examples/
│
└── FreeListAllocator/
    ├── include/
    ├── tests/
    ├── benchmarks/
    └── examples/

Build

Compile examples, tests, or benchmarks using a C++23 compiler.

Example:

g++ -std=c++23 tests/test.cpp -Iinclude -o tests/test

Purpose

This project was created to deepen understanding of:

- Manual memory management
- Allocation strategies
- Memory alignment
- Fragmentation
- Object lifetime management
- Performance tradeoffs between allocator designs

License

This project is provided for educational purposes.