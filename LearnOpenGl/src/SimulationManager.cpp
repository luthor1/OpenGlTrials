#include "SimulationManager.h"

void SimulationManager::RegisterSimulation(std::unique_ptr<ISimulation> sim) {
    m_Simulations.push_back(std::move(sim));
}

void SimulationManager::SwitchTo(int index) {
    if (index < 0 || index >= m_Simulations.size()) return;
    
    if (m_IsRunning) Stop();
    if (m_ActiveSim) m_ActiveSim->Shutdown();
    
    m_ActiveIndex = index;
    m_ActiveSim = m_Simulations[index].get();
    m_ActiveSim->Initialize();
}

void SimulationManager::RenderCurrentSimulation() {
    if (m_ActiveSim) m_ActiveSim->Render();
}
