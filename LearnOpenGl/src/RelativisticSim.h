#pragma once
#include "ISimulation.h"
#include "Engine/Renderer.h"
#include "Shader.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

struct RelObject {
    glm::vec3 pos;
    glm::vec3 vel;
    float mass;
    float radius;
    glm::vec3 color;
    std::string name;
};

class RelativisticSim : public ISimulation {
public:
    RelativisticSim() : m_VAO(0), m_VBO(0), m_GridVAO(0), m_GridVBO(0), m_GridEBO(0), m_GridIndexCount(0) {}
    virtual ~RelativisticSim() { Shutdown(); }

    std::string GetName() const override { return "Relativistic Black Hole (SI Physics)"; }
    std::string GetDescription() const override { return "Real-world physics with Sagittarius A* mass, SI units, and N-Body gravity."; }

    void Initialize() override;
    void Update(float dt) override;
    void Render() override;
    void OnSetupUI() override {}
    void OnRuntimeUI() override;
    void Restart() override;
    void Shutdown() override;

private:
    std::shared_ptr<Shader> m_Shader;
    std::shared_ptr<Shader> m_GridShader;
    std::shared_ptr<Shader> m_PlanetShader;
    
    unsigned int m_CameraUBO, m_DiskUBO, m_ObjectsUBO;

    unsigned int m_VAO, m_VBO;
    
    // Warped Grid
    unsigned int m_GridVAO, m_GridVBO, m_GridEBO;
    int m_GridIndexCount;

    // Planets Mesh
    unsigned int m_PlanetVAO, m_PlanetVBO, m_PlanetEBO;
    int m_PlanetIndexCount;

    // Physical Constants (SI)
    const double c = 299792458.0;
    const double G = 6.67430e-11;
    const double M_SagA = 8.54e36; 
    double m_Rs; 

    std::vector<RelObject> m_Objects;
    float m_CamSpeed = 1.0f;
    bool m_FKeyPressed = false;
    float m_TimeScale = 1000.0f;

    void CreateQuad();
    void GenerateGrid();
    void UpdatePhysics(float dt);
    void CreatePlanetMesh();
    void SyncUBOs();
};
