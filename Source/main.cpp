#include "EngineApplication.hpp"
#include "./Core/Math/Math.hpp"
#include "iostream"

int main(){
    RandEngine::Engine::EngineApplication engine;
    engine.Init();
    engine.Run();

    return 0;
}