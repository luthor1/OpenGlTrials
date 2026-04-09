#pragma once
#include "ISimulation.h"
#include <vector>
#include <glm/glm.hpp>

struct MasterpieceParticle {
    glm::vec3 Position;
    glm::vec3 Velocity;
    glm::vec3 Force;
    float Density;
    float Pressure;
    glm::vec3 Color;
};

class FluidSimMasterpiece : public ISimulation {
public:
    FluidSimMasterpiece();
    virtual ~FluidSimMasterpiece();

    std::string GetName() const override { return "SPH Masterpiece (Cinematic Fluid)"; }
    std::string GetDescription() const override { return "Advanced fluid simulation with Surface Tension, Viscosity, and PBR-like visuals."; }

    void OnSetupUI() override;
    void Initialize() override;
    void Update(float dt) override;
    void Render() override;
    void OnRuntimeUI() override;
    void Restart() override;
    void Shutdown() override;

private:
    int m_Count = 8000;
    float m_SmoothingRadius = 0.15f;
    float m_RestDensity = 1000.0f;
    float m_GasConstant = 2500.0f;
    float m_Viscosity = 150.0f;
    float m_SurfaceTension = 0.05f;
    float m_Gravity = 9.81f;
    float m_ParticleScale = 0.08f;

    std::vector<MasterpieceParticle> m_Particles;

    // OpenGL
    unsigned int m_VAO, m_VBO, m_IBO, m_InstPosVBO, m_InstColorVBO;
    int m_IndexCount;

    void ComputeDensityPressure();
    void ComputeForces();
    void Integrate(float dt);
};
