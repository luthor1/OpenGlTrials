#pragma once
#include "ISimulation.h"
#include <vector>
#include <glm/glm.hpp>

struct Spring {
    int p1, p2;
    float restLength;
    float stiffness;
    float damping;
};

struct SoftParticle {
    glm::vec3 Position;
    glm::vec3 OldPosition;
    glm::vec3 Velocity;
    glm::vec3 Acceleration;
    float Mass;
};

class SoftBodySim : public ISimulation {
public:
    SoftBodySim();
    virtual ~SoftBodySim();

    std::string GetName() const override { return "3D Soft Body (Mass-Spring)"; }
    std::string GetDescription() const override { return "Deformable jello physics with structural and shear springs."; }

    void OnSetupUI() override;
    void Initialize() override;
    void Update(float dt) override;
    void Render() override;
    void OnRuntimeUI() override;
    void Restart() override;
    void Shutdown() override;

private:
    int m_Dim = 5; // Cube of 5x5x5
    float m_Stiffness = 500.0f;
    float m_Damping = 2.0f;
    float m_Gravity = 9.81f;

    std::vector<SoftParticle> m_Particles;
    std::vector<Spring> m_Springs;

    // OpenGL
    unsigned int m_VAO, m_VBO, m_IBO, m_InstPosVBO;
    int m_IndexCount;

    void CreateCube();
    void AddSpring(int i, int j, float stiffness);
    void SolveSprings();
    void Integrate(float dt);
};
