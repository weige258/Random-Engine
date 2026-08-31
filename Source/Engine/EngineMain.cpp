#include "EngineMain.hpp"
#include "Systems/System.hpp"
#include "Core/Time/Timer.hpp"
#include <iostream>


namespace RandEngine::Engine
{
        
        void EngineMain::Init()
        {
                system = RandEngine::Core::Memory::MasterPtr<RandEngine::Systems::System>(new RandEngine::Systems::System());
                this->system->Init(*system);
        }

        void EngineMain::Run()
        {
                while (is_running)
                {
                        this->system->Run(*system);
                }
        }

        void EngineMain::Destroy()
        {
                this->system->Destroy();
        }

}