#pragma once

//
// Some routines to handle BLPath objects
//

#include <cstdint>
#include <functional>
#include <concepts>
#include <ranges>
#include <type_traits>
#include <utility>
#include <cmath>

#include "blend2d.h"

namespace waavs {
	static BLRect pathBounds(const BLPath& path)
	{
		BLBox bbox{};
		path.getBoundingBox(&bbox);

		return BLRect(bbox.x0, bbox.y0, bbox.x1 - bbox.x0, bbox.y1 - bbox.y0);
	}
}

namespace waavs {



	// Attributes for an individual command
	struct PathCommandState {
		int fError{ 0 };
		size_t fOffset{ 0 };
		BLPoint fCurrentPoint{};
		uint8_t fCurrentCmd{};

		PathCommandState() = default;

		void reset(size_t offset, uint8_t cmd, const BLPoint& pt, int err = 0)
		{
			fError = err;
			fOffset = offset;
			fCurrentCmd = cmd;
			fCurrentPoint = pt;
		}

		bool isValid() const { return fError == 0; }

		size_t offset() const { return fOffset; }							// offset into command and vertex arrays
		uint8_t command() const { return fCurrentCmd; }					// last command we saw
		const BLPoint& point() const { return fCurrentPoint; }			// last point we saw
	};



	// PathCommandIterator
	struct PathCommandIterator {
		// Standard iterator implementation
		using iterator_category = std::input_iterator_tag;
		using value_type = PathCommandState;
		using difference_type = std::ptrdiff_t;
		using pointer = PathCommandState*;
		using reference = PathCommandState&;


		//PathIterState iterState{};	// internal iterator state
		int fError{};
		size_t fOffset{ 0 };
		size_t numVerts{ 0 };
		const uint8_t* fCmdBegin{ nullptr };
		const uint8_t* fCmdEnd{ nullptr };
		const BLPoint* fVertices{ nullptr };


		mutable PathCommandState fCurrentCommand;

		PathCommandIterator() = default;


		explicit PathCommandIterator(const BLPath& apath, bool atEnd = false)
		{
			fError = 0;
			fOffset = 0;
			fCmdBegin = apath.commandData();
			fCmdEnd = apath.commandDataEnd();
			fVertices = apath.vertexData();
			numVerts = apath.size();

			fCurrentCommand.reset(fOffset, fCmdBegin[fOffset], fVertices[fOffset], fError);

			if (atEnd)
				resetToEnd();
		}

		// Return whether the current position is valid
		explicit operator bool() const { return fOffset < numVerts;}

		// This == operator MUST be implemented, or C++ 20 will
		// not be happy, as != is NOT enough
		// to be safe, and maximally compatible, we implement both
		bool operator==(const PathCommandIterator& other) const 
		{
			return fOffset == other.fOffset;
		}

		bool operator!=(const PathCommandIterator& other) const {
			return !(*this == other);
		}

		void resetToEnd()
		{
			fOffset = numVerts;
			fError = 1;
			fCurrentCommand.reset(fOffset, 0, {NAN, NAN}, fError);
		}

		// Advance to the next position
		bool next()
		{
			if (fOffset >= numVerts)
				return false;

			fOffset++;

			if (fOffset < numVerts)
			{
				fCurrentCommand.reset(fOffset, fCmdBegin[fOffset], fVertices[fOffset], 0);
				return true;
			}
			else
			{
				fCurrentCommand.fError = 1;
				return false;
			}

		}

		// ++iter
		PathCommandIterator& operator++() 
		{ 
			next();
			return *this; 
		}		
		
		// iter++
		PathCommandIterator operator++(int i) 
		{ 
			PathCommandIterator tmp = *this; 	
			next();
			return tmp; 
		}

		// de-reference to get the current value
		const PathCommandState & operator*() const {return fCurrentCommand;}
		const PathCommandState* operator->() const { return &fCurrentCommand; }
	};

	// PathCommandContainer
	// Provides a range-based interface to the commands in a path
	//
	struct PathCommandContainer {
		const BLPath& fPath;

		explicit PathCommandContainer(const BLPath& apath)
			: fPath(apath)
		{
		}

		PathCommandIterator begin() const {
			return PathCommandIterator(fPath);
		}

		PathCommandIterator end() const {
			return PathCommandIterator(fPath, true);
		}
	};
	
	//
	// FilteredPathCommands
	// A container that allows range-based iteration over the commands 
	// in a path, with a predicate to select which ones to include
	// Requires C++ 20
	//
	template <typename Predicate>
	struct FilteredPathCommands {
		using FilteredRange = std::ranges::filter_view<std::ranges::ref_view<PathCommandContainer>, Predicate>;

		PathCommandContainer baseContainer;
		Predicate predicate;
		mutable FilteredRange filteredView;

		explicit FilteredPathCommands(PathCommandContainer container, Predicate pred)
			: baseContainer(std::move(container)),
			predicate(std::move(pred)),  // Store the predicate safely
			filteredView(std::ranges::views::all(baseContainer), predicate) {
		}

		auto begin() const { return std::ranges::begin(filteredView); }
		auto end() const { return std::ranges::end(filteredView); }
	};

}
