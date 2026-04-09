#pragma once
#include "ISimulation.h"
#include <vector>
#include <glm/glm.hpp>

struct FluidParticle {
    glm::vec3 Position;
    glm::vec3 Velocity;
    float Density;
    float Pressure;
    glm::vec3 Force;
};

class FluidSimSPH : public ISimulation {
public:
    FluidSimSPH();
    virtual ~FluidSimSPH();

    std::string GetName() const override { return "3D SPH Fluid (Navier-Stokes)"; }
    std::string GetDescription() const override { return "High-fidelity liquid physics using Smoothed Particle Hydrodynamics."; }

    void OnSetupUI() override;
    void Initialize() override;
    void Update(float dt) override;
    void Render() override;
    void OnRuntimeUI() override;
    void Restart() override;
    void Shutdown() override;

private:
    int m_Count = 5000;
    float m_SmoothingRadius = 0.2f;
    float m_RestDensity = 1000.0f;
    float m_GasConstant = 2000.0f;
    float m_Viscosity = 250.0f;
    float m_Gravity = 9.81f;
    float m_ParticleScale = 0.1f;

    std::vector<FluidParticle> m_Particles;

    // OpenGL
    unsigned int m_VAO, m_VBO, m_IBO, m_InstPosVBO;
    int m_IndexCount;

    void ComputeDensityPressure();
    void ComputeForces();
    void Integrate(float dt);
};
