#include "EngineMain.hpp"
#include "Core/Objects/BaseObject/BaseObject.hpp"
#include "Core/Behaviors/BaseBehavior/BindBaseBehavoir.hpp"
#include "Core/Math/Math.hpp"
#include <iostream>
#include <atomic>
#include <chrono>
#include <thread>
#include <cstdio>
#include <cmath>

int main(){
    RandEngine::Engine::EngineMain app;
    app.Init();
    app.Run();
    app.Destroy();

    return 1;
}