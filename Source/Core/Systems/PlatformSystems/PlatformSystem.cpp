#include "PlatformSystem.hpp"

namespace RandEngine::Core::Systems::PlatformSystems{
      void PlatformSystem::Init(){
          window_manager = Platform::Manager::WindowManager();
          window_manager.Init();
      }

      void PlatformSystem::Run(){
           window_manager.Run();
      }

      void PlatformSystem::Destroy(){
           window_manager.Destroy();
      }

}