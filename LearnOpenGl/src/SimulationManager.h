#pragma once
#include <memory>
#include <vector>
#include "ISimulation.h"
#include "Camera.h"

class SimulationManager {
public:
    static SimulationManager& Get() {
        static SimulationManager instance;
        return instance;
    }

    void RegisterSimulation(std::unique_ptr<ISimulation> sim);
    void SwitchTo(int index);
    
    ISimulation* GetActive() { return m_ActiveSim; }
    const std::vector<std::unique_ptr<ISimulation>>& GetSimulations() const { return m_Simulations; }
    int GetActiveIndex() const { return m_ActiveIndex; }

    Camera& GetCamera() { return m_Camera; }

    bool IsRunning() const { return m_IsRunning; }
    bool IsPaused() const { return m_IsPaused; }
    
    void TogglePause() { m_IsPaused = !m_IsPaused; }
    void Start() { m_IsRunning = true; m_IsPaused = false; }
    void Stop() { m_IsRunning = false; m_IsPaused = false; }
    void Restart() { if (m_ActiveSim) m_ActiveSim->Restart(); }

private:
    SimulationManager() : m_ActiveSim(nullptr), m_ActiveIndex(-1), m_IsRunning(false), m_IsPaused(false) {}
    
    std::vector<std::unique_ptr<ISimulation>> m_Simulations;
    ISimulation* m_ActiveSim;
    int m_ActiveIndex;
    bool m_IsRunning;
    bool m_IsPaused;
    Camera m_Camera;
};
