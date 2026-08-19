#pragma once 
#include "Platform/Manager/WindowManager.hpp"

namespace RandEngine::Core::Systems::PlatformSystems{
       
    class PlatformSystem
    {
        Platform::Manager::WindowManager window_manager;

        public:
            void Init();

            void Run();

            void Destroy();
    } ;
}