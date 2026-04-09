#pragma once
#include <string>

class ISimulation {
public:
    virtual ~ISimulation() {}

    virtual std::string GetName() const = 0;
    virtual std::string GetDescription() const = 0;

    // Lifecycle
    virtual void OnSetupUI() = 0;       // Before initialization
    virtual void Initialize() = 0;     // Allocate resources
    virtual void Update(float dt) = 0; // Physics / Logic
    virtual void Render() = 0;         // Drawing
    virtual void OnRuntimeUI() = 0;    // While running
    virtual void Restart() = 0;        // Re-initialize without full shutdown
    virtual void Shutdown() = 0;       // Cleanup
};
