#include "EngineApplication.hpp"
#include "./Core/Math/Math.hpp"
#include "iostream"

int main(){
    RandEngine::Engine::EngineApplication engine;
    engine.Init();
    engine.Run();

    RandEngine::Core::Math::Vec<float, 3> vec = {1.0f, 2.0f, 3.0f};

    std::cout << vec * RandEngine::Core::Math::Mat<float, 3,3>::MakeIdentity()  << std::endl;

    return 0;
}