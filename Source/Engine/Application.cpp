#include "Application.hpp"
#include "Systems/System.hpp"
#include "Core/Time/Timer.hpp"
#include <iostream>


namespace RandomEngine::Engine
{
        
        void Application::Init()
        {
                system = RandomEngine::Core::Memory::MasterPtr<RandomEngine::Systems::System>(new RandomEngine::Systems::System());
                this->system->Init(*system.Get());
        }

        void Application::Run()
        {
                while (is_running)
                {
                        this->system->Run(*system.Get());
                }
        }

        void Application::Destroy()
        {
                this->system->Destroy();
        }

}