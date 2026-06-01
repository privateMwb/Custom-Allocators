#pragma once 

#include "ArenaAllocator.h"

class ArenaScope {
    private:
    // Reference 
    ArenaAllocator& arena;
    
    public:
    // Constructor & Destructor
    explicit ArenaScope(ArenaAllocator& arena) :
    arena(arena) {
        arena.beginFrame();
    }
    
    ~ArenaScope() {
        arena.endFrame();
    }
};