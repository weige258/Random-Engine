#include "EngineApplication.hpp"
#include "Core/Objects/BaseObject/BaseObject.hpp"
#include "Core/Behaviors/BaseBehavior/ILogicUpdateBehavior.hpp"
#include "Memory/ObserverPtr.hpp"
#include "Core/Objects/Windows/Window.hpp"
#include "iostream"
using namespace std;

int main()
{


    RandEngine::Engine::EngineApplication app;
    app.Init();
    //app.system->resource_system.object_system.Add(RandEngine::Core::Objects::Windows::Window());
    app.Run(); 
    app.Destroy();
    return 0;
}