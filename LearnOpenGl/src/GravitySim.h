#pragma once
#include "ISimulation.h"
#include <vector>

class GravitySim : public ISimulation {
public:
    GravitySim();
    virtual ~GravitySim();

    std::string GetName() const override { return "Newtonian Gravity (2D)"; }
    std::string GetDescription() const override { return "Particles falling and bouncing with SIMD gravity."; }

    void OnSetupUI() override;
    void Initialize() override;
    void Update(float dt) override;
    void Render() override;
    void OnRuntimeUI() override;
    void Restart() override;
    void Shutdown() override;

private:
    int m_Count = 100000;
    float m_Gravity = 9.8f;
    float m_Friction = 0.99f;
    float m_Bounciness = 0.7f;
    float m_ParticleSize = 2.0f;
    bool m_UseSIMD = true;

    float* m_PosX;
    float* m_PosY;
    float* m_VelX;
    float* m_VelY;

    unsigned int m_VAO, m_VBO, m_InstanceVBO;
    std::vector<float> m_InterleavedPos;

    void UpdateScalar(float dt);
    void UpdateSIMD(float dt);
};
