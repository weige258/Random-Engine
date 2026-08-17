#pragma once

#include "Systems/ResourceSystems/ResourceSystem.hpp"

namespace RandEngine::Core::Systems{
    

struct System{
    
    RandEngine::Core::Systems::ResourceSystems::ResourceSystem resource_system ;
    
    void Init();

    void Run();

    void Destory();
};

}