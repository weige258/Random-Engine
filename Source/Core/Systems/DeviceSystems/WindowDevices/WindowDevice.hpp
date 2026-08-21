#pragma once
#include <vector>
#include <memory>
#include "string"
#include "variant"
#include "Platform/Window/IWindow.hpp"

namespace RandEngine::Core::Systems::DeviceSystems::WindowDevices
{
    class WindowDevice
    {
    private:
         std::vector<std::unique_ptr<Platform::Window::IWindow>> windows;

    public:
        bool AddWindow(const Platform::Window::WindowDesc& desc = {});

        bool DeleteWindow(std::variant<Platform::Window::IWindow*, std::string,size_t> window);
        
        void Init();
        void Run();
        void Destroy();
    };
}