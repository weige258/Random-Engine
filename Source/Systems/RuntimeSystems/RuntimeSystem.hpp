#pragma once
#include "Systems/ISystem.hpp"
#include "Systems/RuntimeSystems/LogicUpdateSystem.hpp"

namespace RandEngine::Systems::RuntimeSystems
{ 

class RuntimeSystem:Systems::ISystem
{
private:
    LogicUpdateSystem logic_update_system;

public:

    void Init(System& system);

    void Run(System& system);

    void Destroy();
};

}

