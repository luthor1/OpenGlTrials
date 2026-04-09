#pragma once
#include "ISimulation.h"
#include <vector>
#include <glm/glm.hpp>

class GalaxySim3D : public ISimulation {
public:
    GalaxySim3D();
    virtual ~GalaxySim3D();

    std::string GetName() const override { return "Galaxy N-Body (3D AVX2)"; }
    std::string GetDescription() const override { return "3D gravitational cluster simulator using high-performance SIMD."; }

    void OnSetupUI() override;
    void Initialize() override;
    void Update(float dt) override;
    void Render() override;
    void OnRuntimeUI() override;
    void Restart() override;
    void Shutdown() override;

private:
    int m_Count = 50000;
    float m_G = 0.0001f;
    float m_Softening = 0.01f;
    float m_Scale = 0.1f;

    // SOA Layout (32-byte aligned for AVX)
    float* m_PosX;
    float* m_PosY;
    float* m_PosZ;
    float* m_VelX;
    float* m_VelY;
    float* m_VelZ;
    float* m_ColorR;
    float* m_ColorG;
    float* m_ColorB;

    unsigned int m_VAO, m_VBO, m_IBO;
    unsigned int m_InstPosVBO, m_InstColorVBO;
    int m_IndexCount;

    void UpdateSIMD(float dt);
};
