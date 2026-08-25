#include "EngineMain.hpp"
#include "iostream"
#include "Core/Behaviors/BaseBehavior/BaseBehavior.hpp"
#include "Core/Behaviors/BaseBehavior/ILogicUpdateBehavior.hpp"
using namespace std;
#include <random>

int main()
{

    RandEngine::Engine::EngineMain app;
    app.Init();
    app.Run();
    app.Destroy();
    return 0;
}