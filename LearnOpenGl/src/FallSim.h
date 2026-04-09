#pragma once
#include "ISimulation.h"
#include <vector>

class FallSim : public ISimulation {
public:
    FallSim();
    virtual ~FallSim();

    std::string GetName() const override { return "Scientific Fall Simulator"; }
    std::string GetDescription() const override { return "A precise vacuum fall experiment with metrics."; }

    void OnSetupUI() override;
    void Initialize() override;
    void Update(float dt) override;
    void Render() override;
    void OnRuntimeUI() override;
    void Restart() override;
    void Shutdown() override;

private:
    // Parameters
    float m_StartHeight = 100.0f; // meters
    float m_Gravity = 9.81f;    // m/s^2
    float m_Mass = 1.0f;       // kg
    float m_MeterScale = 10.0f; // 1 screen unit = 10 meters

    // State
    float m_CurrentHeight;     // meters
    float m_Velocity;          // m/s
    float m_Time;              // s
    bool m_Impacted;
    
    // Stats
    float m_ImpactTime;
    float m_MaxVelocity;

    unsigned int m_VAO, m_VBO;
    void DrawGround();
    void DrawObject();
};
