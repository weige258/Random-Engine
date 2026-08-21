#include "WindowDevice.hpp"
#include <stdexcept>
#include "Window/SDLWindow.hpp"
#include "SDL3/SDL.h"

#if defined(_WIN32)
    // #include "Windows/Win32Window.hpp"
#elif defined(__APPLE__)
    // #include "Windows/CocoaWindow.hpp"
#endif

namespace RandEngine::Core::Systems::DeviceSystems::WindowDevices{
    
    bool WindowDevice::AddWindow(const Platform::Window::WindowDesc& desc) {
        std::unique_ptr<Platform::Window::IWindow> new_window = nullptr;

        #if defined(_WIN32)
            new_window = std::make_unique<Platform::Window::SDLWindow>(desc);
        #elif defined(__APPLE__)
            new_window = std::make_unique<Platform::Window::SDLWindow>(desc);
        #else
            new_window = std::make_unique<Platform::Window::SDLWindow>(desc);
        #endif

        if (!new_window) {
            return false;
        }

        windows.push_back(std::move(new_window));
        return true;
    }

    bool WindowDevice::DeleteWindow(std::variant<Platform::Window::IWindow*, std::string,size_t> window){ 
        return std::visit([this](auto&& arg) -> bool {
        using T = std::decay_t<decltype(arg)>;

        // 1. 传入的是索引 (size_t)
        if constexpr (std::is_same_v<T, size_t>) {
            if (arg < windows.size()) {
                windows.erase(windows.begin() + arg);
                return true;
            }
            return false;
        }
        // 2. 传入的是窗口指针 (IWindow*)
        else if constexpr (std::is_same_v<T, Platform::Window::IWindow*>) {
            return std::erase_if(windows, [arg](const auto& win) {
                return win.get() == arg;
            }) > 0;
        }
        // 3. 传入的是窗口名称/标题 (std::string)
        else if constexpr (std::is_same_v<T, std::string>) {
            return std::erase_if(windows, [&arg](const auto& win) {
                return win->GetDesc().title == arg; 
            }) > 0;
        }

        return false;
    }, window);
    }

    void WindowDevice::Init(){
        if(!SDL_Init(SDL_INIT_VIDEO)){
            throw std::runtime_error("Failed to initialize SDL: " + std::string(SDL_GetError()));
        }

        windows.push_back(std::make_unique<Platform::Window::SDLWindow>(Platform::Window::SDLWindow(Platform::Window::WindowDesc())));
    }

    void WindowDevice::Run(){
        for(size_t i=0; i<windows.size(); i++){
            if(windows[i]->ShouldClose()){
                windows.erase(windows.begin() + i);
                break;
            }
            windows[i]->PollEvents();
        }
    }

    void WindowDevice::Destroy(){
        windows.clear();
    }
}