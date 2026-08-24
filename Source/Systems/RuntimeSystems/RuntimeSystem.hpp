#pragma once
#include "Systems/ISystem.hpp"

namespace RandEngine::Systems::RuntimeSystems
{ 

class RuntimeSystem:Systems::ISystem
{
private:
    
public:

    void Init();

    void Run(System& system);

    void Destroy();
};

}

