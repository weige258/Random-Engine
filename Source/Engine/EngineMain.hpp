#include "Systems/System.hpp"
#include "Core/Memory/MasterPtr.hpp"
#pragma once

namespace RandEngine::Engine
{

    class EngineMain
    {

    private:
        bool is_running = true;
 
    public:
        
        static inline RandEngine::Core::Memory::MasterPtr<RandEngine::Systems::System> system ;

        void Init();

        void Run();

        void Destroy();
    };
}