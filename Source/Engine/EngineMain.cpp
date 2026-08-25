#include "EngineMain.hpp"
#include "Systems/System.hpp"

namespace RandEngine::Engine
{

        void EngineMain::Init()
        {
                system = new RandEngine::Systems::System();
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
                delete this->system;
        }

}