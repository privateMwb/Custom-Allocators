#pragma once 

#include "StackAllocator.h"

class StackScope {
    private:
    // Reference
    StackAllocator& stack;
    size_t marker;
    
    public:
    // Constructors & Destructor
    explicit StackScope(StackAllocator& stack) : 
    stack(stack),
    marker(stack.getMarker()) {}
    
    ~StackScope() {
        stack.freeToMarker(marker);
    }
    
    StackScope(const StackScope& other) = delete;
    StackScope& operator=(const StackScope& other) = delete;
};
