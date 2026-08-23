#include "EngineMain.hpp"
#include "iostream"
using namespace std;

int main()
{

    RandEngine::Engine::EngineMain app;
    app.Init();
    app.Run(); 
    app.Destroy();
    return 0;
}