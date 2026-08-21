#include "EngineMain.hpp"
#include "Systems/System.hpp"

namespace RandEngine::Engine
{

        void EngineMain::Init()
        {
                system = new RandEngine::Core::Systems::System();
                this->system->Init();
        }

        void EngineMain::Run()
        {
                while (is_running)
                {
                        this->system->Run();
                }
        }

        void EngineMain::Destroy()
        {
                this->system->Destroy();
                delete this->system;
        }

}