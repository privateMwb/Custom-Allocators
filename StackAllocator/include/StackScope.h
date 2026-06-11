#pragma once

#include "StackAllocator.h"

class [[nodiscard]] StackScope {
private:
    // Reference
    StackAllocator& stack;
    std::size_t     marker;

public:
    // Constructors & Destructor
    explicit StackScope(StackAllocator& stack)
        : stack(stack)
        , marker(stack.getMarker())
    {}

    ~StackScope() noexcept {
        stack.freeToMarker(marker);
    }

    StackScope(const StackScope&)            = delete;
    StackScope& operator=(const StackScope&) = delete;

    StackScope(StackScope&&)                 = delete;
    StackScope& operator=(StackScope&&)      = delete;
};