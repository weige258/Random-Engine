#pragma once
#include "Systems/ISystem.hpp"
#include "Systems/RuntimeSystems/LogicUpdateSystem.hpp"
#include "Systems/RuntimeSystems/FixUpdateSystem.hpp"

namespace RandomEngine::Systems::RuntimeSystems
{ 

class RuntimeSystem:Systems::ISystem
{
private:
    LogicUpdateSystem logic_update_system;

public:
    FixUpdateSystem fix_update_system;

    void Init(System& system);

    void Run(System& system);

    void Destroy();
};

}