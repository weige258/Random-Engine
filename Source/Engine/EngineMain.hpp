#include "Systems/System.hpp"

#pragma once

namespace RandEngine::Engine
{

    class EngineMain
    {

    private:
        bool is_running = true;

    public:
        static inline RandEngine::Core::Systems::System *system = nullptr;

        void Init();

        void Run();

        void Destroy();
    };
}