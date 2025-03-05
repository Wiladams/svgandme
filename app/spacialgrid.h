#pragma once

#include <unordered_map>
#include <vector>
#include <cmath>
#include <algorithm>

#include "agraphic.h"

namespace waavs {
    struct GridIndex {
        int x, y;

        bool operator==(const GridIndex& other) const {
            return x == other.x && y == other.y;
        }
    };

    // Hash function for GridIndex
    namespace std {
        template <>
        struct hasher<GridIndex> {
            size_t operator()(const GridIndex& key) const {
                return hasher<int>()(key.x) ^ (hasher<int>()(key.y) << 1);
            }
        };
    }

    class SpatialGrid {
    public:
        float cellSize;
        std::unordered_map<GridIndex, std::vector<AGraphicHandle>> grid;
        std::vector<AGraphicHandle> insertionOrder; // Keeps global insertion order

        explicit SpatialGrid(float cellSize = 100.0f) : cellSize(cellSize) {}

        // Convert world coordinate to grid cell index
        GridIndex toGridCell(float x, float y) const {
            return { static_cast<int>(std::floor(x / cellSize)), static_cast<int>(std::floor(y / cellSize)) };
        }

        void insert(AGraphicHandle obj) {
            BLRect bbox = obj->frame();

            int xStart = std::floor(bbox.x / cellSize);
            int xEnd = std::floor((bbox.x + bbox.w) / cellSize);
            int yStart = std::floor(bbox.y / cellSize);
            int yEnd = std::floor((bbox.y + bbox.h) / cellSize);

            for (int gx = xStart; gx <= xEnd; ++gx) {
                for (int gy = yStart; gy <= yEnd; ++gy) {
                    grid[{gx, gy}].push_back(obj);
                }
            }

            insertionOrder.push_back(obj); // Preserve painter’s order
        }

        std::vector<AGraphicHandle> query(float x, float y) {
            GridIndex cell = toGridCell(x, y);
            auto it = grid.find(cell);
            if (it != grid.end()) {
                return it->second;  // Return list of candidates
            }
            return {};
        }

        std::vector<AGraphicHandle> findGraphicsAtPoint(float x, float y) {
            auto candidates = query(x, y);

            // Filter by precise hit test (transform x, y into object space if needed)
            std::vector<AGraphicHandle> hits;
            for (const auto& obj : insertionOrder) {
                if (std::find(candidates.begin(), candidates.end(), obj) != candidates.end()) {
                    BLPoint localPoint{ x, y }; // Transform if necessary
                    if (obj->contains(localPoint)) {
                        hits.push_back(obj);
                    }
                }
            }

            return hits; // In painter’s order: back-to-front
        }
    };
}