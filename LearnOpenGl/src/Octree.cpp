#include "Octree.h"
#include <glm/gtx/norm.hpp>

Octree::Octree(glm::vec3 center, float size) {
    m_Root = std::make_unique<Node>(center, size);
}

Octree::~Octree() {}

void Octree::Insert(const OctreeBody& body) {
    InsertInternal(m_Root.get(), body);
}

void Octree::Query(glm::vec3 pos, float radius, std::vector<OctreeBody>& outNeighbors) {
    QueryInternal(m_Root.get(), pos, radius, outNeighbors);
}

void Octree::Clear() {
    glm::vec3 c = m_Root->Center;
    float s = m_Root->Size;
    m_Root = std::make_unique<Node>(c, s);
}

void Octree::Node::Subdivide() {
    float h = Size * 0.5f;
    float q = Size * 0.25f;
    Children[0] = std::make_unique<Node>(Center + glm::vec3(-q, -q, -q), h);
    Children[1] = std::make_unique<Node>(Center + glm::vec3( q, -q, -q), h);
    Children[2] = std::make_unique<Node>(Center + glm::vec3(-q,  q, -q), h);
    Children[3] = std::make_unique<Node>(Center + glm::vec3( q,  q, -q), h);
    Children[4] = std::make_unique<Node>(Center + glm::vec3(-q, -q,  q), h);
    Children[5] = std::make_unique<Node>(Center + glm::vec3( q, -q,  q), h);
    Children[6] = std::make_unique<Node>(Center + glm::vec3(-q,  q,  q), h);
    Children[7] = std::make_unique<Node>(Center + glm::vec3( q,  q,  q), h);
    IsLeaf = false;

    for (auto& b : Bodies) {
        // We'll re-insert them later or just simplify for now
    }
}

void Octree::InsertInternal(Node* node, const OctreeBody& body) {
    if (node->IsLeaf) {
        if (node->Bodies.size() < 8 || node->Size < 0.1f) {
            node->Bodies.push_back(body);
            return;
        }
        node->Subdivide();
    }

    // Find child
    int index = 0;
    if (body.Position.x > node->Center.x) index |= 1;
    if (body.Position.y > node->Center.y) index |= 2;
    if (body.Position.z > node->Center.z) index |= 4;
    InsertInternal(node->Children[index].get(), body);
}

void Octree::QueryInternal(Node* node, glm::vec3 pos, float radius, std::vector<OctreeBody>& outNeighbors) {
    // Check sphere-box intersection
    float distSq = 0.0f;
    for (int i = 0; i < 3; i++) {
        float v = pos[i];
        float min = node->Center[i] - node->Size * 0.5f;
        float max = node->Center[i] + node->Size * 0.5f;
        if (v < min) distSq += (min - v) * (min - v);
        else if (v > max) distSq += (v - max) * (v - max);
    }
    if (distSq > radius * radius) return;

    if (node->IsLeaf) {
        for (auto& b : node->Bodies) {
            float d2 = glm::distance2(pos, b.Position);
            if (d2 < (radius + b.Radius) * (radius + b.Radius)) {
                outNeighbors.push_back(b);
            }
        }
    } else {
        for (int i = 0; i < 8; i++) {
            QueryInternal(node->Children[i].get(), pos, radius, outNeighbors);
        }
    }
}
