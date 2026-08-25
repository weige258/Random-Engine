#include "WindowSystem.hpp"
#include <stdexcept>

#include "SDL3/SDL.h"

namespace RandEngine::Systems::DeviceSystems
{

    bool WindowSystem::AddWindow(const Platform::Window::WindowDesc &desc)
    {
       
        std::unique_ptr<Platform::Window::NativeWindow> new_window = Platform::Window::NativeWindow::Create(desc);

        if (!new_window)
        {
            return false;
        }

        windows.push_back(std::move(new_window));
        return true;
    }

    bool WindowSystem::DeleteWindow(std::variant<Platform::Window::NativeWindow *, std::string, size_t> window)
    {
        return std::visit([this](auto &&arg) -> bool
                          {
            using T = std::decay_t<decltype(arg)>;

            // 1. 传入的是索引 (size_t)
            if constexpr (std::is_same_v<T, size_t>) {
                if (arg < windows.size()) {
                    windows.erase(windows.begin() + arg);
                    return true;
                }
                return false;
            }
            // 2. 传入的是窗口指针 (NativeWindow*)
            else if constexpr (std::is_same_v<T, Platform::Window::NativeWindow*>) {
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

            return false; }, window);
    }

    void WindowSystem::Init()
    {
        AddWindow();
    }

    void WindowSystem::Run()
    {
        for (size_t i = 0; i < windows.size(); i++)
        {
            if (windows[i]->ShouldClose())
            {
                windows.erase(windows.begin() + i);
                break;
            }
            windows[i]->PollEvents();
        }
    }

    void WindowSystem::Destroy()
    {
        windows.clear();
    }
}