#include "Systems/System.hpp"
#include "Core/Memory/MasterPtr.hpp"
#pragma once

namespace RandomEngine::Engine
{

    class EngineMain
    {

    private:
        bool is_running = true;
 
    public:
        
        static inline RandomEngine::Core::Memory::MasterPtr<RandomEngine::Systems::System> system ;

        void Init();

        void Run();

        void Destroy();
    };
}