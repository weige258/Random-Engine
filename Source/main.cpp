#include "EngineApplication.hpp"

using namespace RandEngine::Engine;

int main()
{
    EngineApplication app;
    app.Init();
    app.Run();
    app.Destroy();
    return 0;
}