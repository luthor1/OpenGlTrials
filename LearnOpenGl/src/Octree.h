#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <memory>

struct OctreeNode;

struct OctreeBody {
    glm::vec3 Position;
    float Radius;
    void* UserData;
    int Type; // 0: Fluid, 1: SoftBody, 2: Galaxy
};

class Octree {
public:
    Octree(glm::vec3 center, float size);
    ~Octree();

    void Insert(const OctreeBody& body);
    void Query(glm::vec3 pos, float radius, std::vector<OctreeBody>& outNeighbors);
    void Clear();

private:
    struct Node {
        glm::vec3 Center;
        float Size;
        std::vector<OctreeBody> Bodies;
        std::unique_ptr<Node> Children[8];
        bool IsLeaf = true;

        Node(glm::vec3 c, float s) : Center(c), Size(s) {}
        void Subdivide();
    };

    std::unique_ptr<Node> m_Root;
    void InsertInternal(Node* node, const OctreeBody& body);
    void QueryInternal(Node* node, glm::vec3 pos, float radius, std::vector<OctreeBody>& outNeighbors);
};
