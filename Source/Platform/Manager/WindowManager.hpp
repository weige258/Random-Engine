#pragma once
#include <vector>
#include <memory>
#include "../Windows/IWindow.hpp"

namespace RandEngine::Platform::Manager
{
    class WindowManager
    {
    private:
         std::vector<std::unique_ptr<Windows::IWindow>> windows;

    public:
        void Init();
        void Run();
        void Destroy();
    };
}