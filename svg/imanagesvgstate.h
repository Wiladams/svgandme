#pragma once

#include <functional>

#include "blend2d.h"
#include "fonthandler.h"
#include "collections.h"
#include "svgenums.h"
#include "svgdrawingstate.h"

namespace waavs
{

    struct IManageSVGState : public SVGDrawingState
    {
        // Managing state
        WSStack<SVGDrawingState> fStateStack{};


        SVGDrawingState & operator=(const SVGDrawingState& st) noexcept override
        {
            return SVGDrawingState::operator=(st);
        }

        virtual void onPush() {}
        
        virtual bool push() {
            // Save the current state to the state stack
            fStateStack.push(*this);
            onPush();
            
            return true;
        }

        virtual void onPop() {}
        
        virtual bool pop()
        {
            if (fStateStack.empty())
                return false;

            // Restore the current state from the state stack
			SVGDrawingState st = fStateStack.top();
            
			// Assign the state to the current state
			this->operator=(st);
            
            fStateStack.pop();
            onPop();

            return true;
        }

        virtual void resetState()
        {
            //resetTransform();

            fStateStack.clear();

            reset();
        }

    };
}
