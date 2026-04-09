#pragma once
#include "ISimulation.h"
#include "Engine/Renderer.h"
#include "Shader.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

struct RelObject {
    glm::vec3 pos = glm::vec3(0.0f);
    glm::vec3 vel = glm::vec3(0.0f);
    float mass = 0.0f;
    float radius = 1.0f;
    glm::vec3 color = glm::vec3(1.0f);
    std::string name = "Object";

    RelObject() = default;
};

class RelativisticSim : public ISimulation {
public:
    RelativisticSim() 
        : m_CameraUBO(0), m_DiskUBO(0), m_ObjectsUBO(0),
          m_VAO(0), m_VBO(0), 
          m_GridVAO(0), m_GridVBO(0), m_GridEBO(0), m_GridIndexCount(0),
          m_PlanetVAO(0), m_PlanetVBO(0), m_PlanetEBO(0), m_PlanetIndexCount(0),
          m_Rs(0.0), m_ShowGrid(true), m_FKeyPressed(false), m_TimeScale(1000.0f) {}
    
    virtual ~RelativisticSim() { Shutdown(); }
    
    // ... (GetName / GetDescription same)
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
    
    unsigned int m_GridVAO, m_GridVBO, m_GridEBO;
    int m_GridIndexCount;

    unsigned int m_PlanetVAO, m_PlanetVBO, m_PlanetEBO;
    int m_PlanetIndexCount;

    const double c = 299792458.0;
    const double G = 6.67430e-11;
    const double M_SagA = 8.54e36; 
    double m_Rs; 

    std::vector<RelObject> m_Objects;
    float m_CamSpeed = 50.0f;
    bool m_ShowGrid = true;
    bool m_FKeyPressed = false;
    float m_TimeScale = 1000.0f;

    void CreateQuad();
    void GenerateGrid();
    void UpdatePhysics(float dt);
    void CreatePlanetMesh();
    void SyncUBOs();
};
