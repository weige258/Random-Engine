#include "EngineMain.hpp"
#include "Core/Objects/BaseObject/BaseObject.hpp"
#include "Core/Behaviors/BaseBehavior/BindBaseBehavoir.hpp"
#include "iostream"


int main()
{

    RandEngine::Engine::EngineMain app;
    app.Init();
    app.Run();
    app.Destroy();
    return 0;
}