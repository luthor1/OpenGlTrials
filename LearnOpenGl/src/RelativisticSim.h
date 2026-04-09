#pragma once
#include "ISimulation.h"
#include "Engine/Renderer.h"
#include <memory>
#include "Shader.h"

class RelativisticSim : public ISimulation {
public:
    RelativisticSim() : m_VAO(0), m_VBO(0) {}
    virtual ~RelativisticSim() { Shutdown(); }

    std::string GetName() const override { return "Relativistic Black Hole (GPU)"; }
    std::string GetDescription() const override { return "Ray-traced black hole using RK4 geodesic integration (God-Level Shader)."; }

    void Initialize() override;
    void Update(float dt) override;
    void Render() override;
    void OnSetupUI() override {}
    void OnRuntimeUI() override;
    void Restart() override;
    void Shutdown() override;

private:
    std::shared_ptr<Shader> m_Shader;
    unsigned int m_VAO, m_VBO;

    void CreateQuad();
};
