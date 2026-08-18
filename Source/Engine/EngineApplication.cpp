#include "EngineApplication.hpp"
#include "Systems/System.hpp"

namespace RandEngine::Engine
{
        
        void EngineApplication::Init()
        {
                system = new RandEngine::Core::Systems::System();
                this->system->Init();
        }

        void EngineApplication::Run() {
                while (is_running)
                {
                        this->system->Run();
                }
        }

        void EngineApplication::Destroy()
        {
                this->system->Destory();
                delete this->system;
        }

}