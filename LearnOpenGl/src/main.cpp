#include "Engine/Application.h"
#include "SimulationManager.h"
#include "GravitySim.h"
#include "FallSim.h"
#include "GalaxySim3D.h"
#include "FluidSimSPH.h"
#include "SoftBodySim.h"

int main()
{
    Application app;

    // Register Simulations
    SimulationManager::Get().RegisterSimulation(std::make_unique<SoftBodySim>());
    SimulationManager::Get().RegisterSimulation(std::make_unique<FluidSimSPH>());
    SimulationManager::Get().RegisterSimulation(std::make_unique<GalaxySim3D>());
    SimulationManager::Get().RegisterSimulation(std::make_unique<GravitySim>());
    SimulationManager::Get().RegisterSimulation(std::make_unique<FallSim>());

    app.Run();

    return 0;
}
