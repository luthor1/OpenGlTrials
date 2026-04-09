#pragma once
#include "ISimulation.h"
#include <vector>
#include <glm/glm.hpp>

struct SpaceObject {
    glm::vec3 Position;
    glm::vec3 Velocity;
    float Mass;
    float Radius;
    glm::vec3 Color;
    std::string Name;
};

class SpaceTimeSim : public ISimulation {
public:
    SpaceTimeSim();
    virtual ~SpaceTimeSim();

    std::string GetName() const override { return "Space-Time Curvature Sim"; }
    std::string GetDescription() const override { return "3D Gravity simulation with Flamm Paraboloid space-time grid curvature."; }

    void Initialize() override;
    void Update(float dt) override;
    void Render() override;
    void OnSetupUI() override {}
    void OnRuntimeUI() override;
    void Restart() override;
    void Shutdown() override;

private:
    std::vector<SpaceObject> m_Objects;
    float m_G = 1.0f;
    bool m_ShowGrid = true;

    unsigned int m_GridVAO, m_GridVBO, m_GridEBO;
    int m_GridIndexCount;

    unsigned int m_PlanetVAO, m_PlanetVBO, m_PlanetIBO;
    int m_PlanetIndexCount;

    void CreateGrid(int resolution, float size);
    void CreatePlanetMesh();
};
