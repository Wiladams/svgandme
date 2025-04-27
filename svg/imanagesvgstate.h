#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "blend2d.h"
#include "fonthandler.h"
#include "collections.h"
#include "svgenums.h"
#include "svgdrawingstate.h"

namespace waavs
{
    struct StateMemoryPool 
    {
    public:
        std::vector<std::unique_ptr<SVGDrawingState>> pool;
        size_t currentIndex = 0;

        StateMemoryPool(size_t poolSize = 10) 
            : pool(poolSize) 
        {
            for (size_t i = 0; i < poolSize; ++i) {
                pool[i] = std::make_unique<SVGDrawingState>();
            }
        }

        SVGDrawingState* allocate() {
            if (currentIndex >= pool.size()) {
                // Double the pool size if exhausted
                size_t newSize = pool.size() * 2;
                pool.reserve(newSize);
                for (size_t i = pool.size(); i < newSize; ++i) {
                    pool.push_back(std::make_unique<SVGDrawingState>());
                }
            }
            return pool[currentIndex++].get();
        }

        void reset() { currentIndex = 0; }
    };

    // SVGStateStack
    // A managed stack of drawing state structures
    // This is backed by a memory pool, which will reduce runtime
	// dynamic memory allocation, assuming the default stack size
	// is sufficient for the depth of the SVG tree.
    // If more space is needed, it is automatically allocated
    struct SVGStateStack 
    {
    public:
        std::deque<SVGDrawingState*> stateStack{};
        std::unique_ptr<StateMemoryPool> memoryPool{};
        std::unique_ptr<SVGDrawingState> fCurrentState{};

        SVGStateStack()
            : memoryPool(std::make_unique<StateMemoryPool>(10)),
            fCurrentState(std::make_unique<SVGDrawingState>())
        {}

        // Retrieve a pointer to the current state
        SVGDrawingState* currentState() const 
        {
            return fCurrentState.get();
        }

        // Optimized Push Operation
        void push() {
            if (fCurrentState->isModified() || stateStack.empty())
            {
                SVGDrawingState* newState = memoryPool->allocate();
                *newState = *fCurrentState;  // Shallow copy of the entire state
                stateStack.push_back(newState);
                fCurrentState->modifiedSinceLastPush = false;
            }
        }

        // Optimized Pop Operation
        void pop() {
            if (!stateStack.empty()) {
                fCurrentState.reset();
                fCurrentState = std::make_unique<SVGDrawingState>(*stateStack.back());  // Copy previous state
                stateStack.pop_back();
			}
            else {
				printf("SVGStateStack::pop() ERROR: Stack is empty\n");
                // Reset the current state
                //fCurrentState.reset();
                //fCurrentState = std::make_unique<SVGDrawingState>();
            }
        }

        // Reset the entire state stack
        void reset() {
            stateStack.clear();
            memoryPool->reset();
            fCurrentState = std::make_unique<SVGDrawingState>();
        }
    };


}
