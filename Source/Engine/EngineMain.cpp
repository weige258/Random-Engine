#include "EngineMain.hpp"
#include "Systems/System.hpp"
#include "Core/Time/Timer.hpp"
#include <iostream>


namespace RandomEngine::Engine
{
        
        void EngineMain::Init()
        {
                system = RandomEngine::Core::Memory::MasterPtr<RandomEngine::Systems::System>(new RandomEngine::Systems::System());
                this->system->Init(*system.Get());
        }

        void EngineMain::Run()
        {
                while (is_running)
                {
                        this->system->Run(*system.Get());
                }
        }

        void EngineMain::Destroy()
        {
                this->system->Destroy();
        }

}