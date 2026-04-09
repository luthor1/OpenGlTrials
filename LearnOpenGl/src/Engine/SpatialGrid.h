#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <unordered_map>

class SpatialGrid {
public:
    SpatialGrid(float cellSize) : m_CellSize(cellSize) {}

    void Clear() {
        m_Grid.clear();
    }

    void Insert(int particleIndex, const glm::vec3& pos) {
        int gx = (int)floor(pos.x / m_CellSize);
        int gy = (int)floor(pos.y / m_CellSize);
        int gz = (int)floor(pos.z / m_CellSize);
        uint64_t key = Hash(gx, gy, gz);
        m_Grid[key].push_back(particleIndex);
    }

    std::vector<int> GetNeighbors(const glm::vec3& pos) {
        std::vector<int> neighbors;
        int gx = (int)floor(pos.x / m_CellSize);
        int gy = (int)floor(pos.y / m_CellSize);
        int gz = (int)floor(pos.z / m_CellSize);

        for (int x = gx - 1; x <= gx + 1; ++x) {
            for (int y = gy - 1; y <= gy + 1; ++y) {
                for (int z = gz - 1; z <= gz + 1; ++z) {
                    uint64_t key = Hash(x, y, z);
                    if (m_Grid.count(key)) {
                        neighbors.insert(neighbors.end(), m_Grid[key].begin(), m_Grid[key].end());
                    }
                }
            }
        }
        return neighbors;
    }

private:
    uint64_t Hash(int x, int y, int z) {
        return ((uint64_t)x * 73856093) ^ ((uint64_t)y * 19349663) ^ ((uint64_t)z * 83492791);
    }

    float m_CellSize;
    std::unordered_map<uint64_t, std::vector<int>> m_Grid;
};
