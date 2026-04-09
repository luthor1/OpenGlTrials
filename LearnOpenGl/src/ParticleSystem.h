#pragma once
#include <vector>
#include <glm/glm.hpp>

class ParticleSystem {
public:
    ParticleSystem(int maxParticles);
    ~ParticleSystem();

    void Update(float dt);
    void Reset();

    // Getters for rendering
    int GetCount() const { return m_MaxParticles; }
    const float* GetPositionsX() const { return m_PosX; }
    const float* GetPositionsY() const { return m_PosY; }

    // Controls
    float Gravity = 9.8f;
    float Friction = 0.99f;
    float Bounciness = 0.7f;
    float ParticleSize = 2.0f;
    bool UseSIMD = true;

private:
    int m_MaxParticles;

    // SOA Layout
    float* m_PosX;
    float* m_PosY;
    float* m_VelX;
    float* m_VelY;

    void UpdateScalar(float dt);
    void UpdateSIMD(float dt);
};
