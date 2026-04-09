#pragma once
#include "ISimulation.h"
#include <vector>
#include <glm/glm.hpp>

struct SoftParticle {
    glm::vec3 Position;
    glm::vec3 OldPosition;
    glm::vec3 Velocity;
    float Mass;
};

struct PBDConstraint {
    int p1, p2;
    float restLength;
};

class SoftBodySim : public ISimulation {
public:
    SoftBodySim();
    virtual ~SoftBodySim();

    std::string GetName() const override { return "Soft Body Masterpiece (PBD)"; }
    std::string GetDescription() const override { return "Advanced Position Based Dynamics with Mesh deformation and smooth shading."; }

    void OnSetupUI() override;
    void Initialize() override;
    void Update(float dt) override;
    void Render() override;
    void OnRuntimeUI() override;
    void Restart() override;
    void Shutdown() override;

private:
    int m_Dim = 6; 
    float m_Compliance = 0.0001f;
    float m_Gravity = 9.81f;

    std::vector<SoftParticle> m_Particles;
    std::vector<PBDConstraint> m_Constraints;
    std::vector<unsigned int> m_MeshIndices;

    // OpenGL
    unsigned int m_VAO, m_VBO, m_IBO, m_NormalVBO;
    int m_IndexCount;

    void CreateSoftBody();
    void AddConstraint(int i, int j);
    void SolveConstraints();
    
    glm::vec3 m_LastSpherePos;
};
