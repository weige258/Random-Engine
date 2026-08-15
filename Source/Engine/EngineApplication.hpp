#include "Systems/System.hpp"


#pragma once

namespace RandEngine::Engine {
    
    class EngineApplication {
        public:

        static inline RandEngine::Core::Systems::System* system = nullptr;

        void Init();

        void Run();

        void Destroy();
    };
}