#include "EngineMain.hpp"
#include "iostream"
#include "Core/Objects/BaseObject/TestEnity.hpp"

int main()
{

    RandEngine::Engine::EngineMain app;
    app.Init();
    for (int i = 0; i < 10000; i++)
    {
        RandEngine::Core::Objects::TestEnity enity;
        app.system->resource_system.Add(enity);
    }
    app.Run();
    app.Destroy();
    return 0;
}