#include "SDLWindow.hpp"
#include <SDL3/SDL.h>
#include <stdexcept>
#include <string>
#include <utility>

namespace RandEngine::Platform::Windows{
     
    SDLWindow::SDLWindow(const WindowDesc& desc)
        : desc(desc)
    {
        const Uint64 sdl_flags = MapFlags(desc.flags, desc.mode);

        window = SDL_CreateWindow(
            desc.title.c_str(),
            desc.width,
            desc.height,
            static_cast<SDL_WindowFlags>(sdl_flags)
        );

        if (!window)
        {
            throw std::runtime_error(std::string("SDL_CreateWindow Failed: ") + SDL_GetError());
        }
    }

    SDLWindow::~SDLWindow()
    {
        if (window)
        {
            SDL_DestroyWindow(window);
            window = nullptr;
        }
    }

    SDLWindow::SDLWindow(SDLWindow&& other) noexcept
        : window(std::exchange(other.window, nullptr)),
          desc(std::move(other.desc)),
          should_close(other.should_close),
          event_callback(std::move(other.event_callback))
    {
    }

    SDLWindow& SDLWindow::operator=(SDLWindow&& other) noexcept
    {
        if (this != &other)
        {
            if (window)
            {
                SDL_DestroyWindow(window);
            }

            window         = std::exchange(other.window, nullptr);
            desc           = std::move(other.desc);
            should_close   = other.should_close;
            event_callback = std::move(other.event_callback);
        }
        return *this;
    }

    void SDLWindow::PollEvents()
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            // 过滤非当前窗口的窗口事件
            if (event.window.windowID != 0 && window && event.window.windowID != SDL_GetWindowID(window))
            {
                continue;
            }

            // 若注册了通用回调，将原生事件指针抛出给引擎事件总线
            if (event_callback)
            {
                event_callback(&event);
            }

            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                should_close = true;
                break;

            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                should_close = true;
                break;

            case SDL_EVENT_WINDOW_RESIZED:
                desc.width  = event.window.data1;
                desc.height = event.window.data2;
                break;

            default:
                break;
            }
        }
    }

    void SDLWindow::SetDesc(const WindowDesc& desc)
    {
        this->desc = desc;

        if (!window)
            return;

        SDL_SetWindowTitle(window, desc.title.c_str());
        SDL_SetWindowSize(window, desc.width, desc.height);

        const bool is_fullscreen = (desc.mode == WindowMode::BorderlessWindowed ||
                                    desc.mode == WindowMode::ExclusiveFullscreen);
        SDL_SetWindowFullscreen(window, is_fullscreen);

        SDL_SetWindowResizable(window, HasFlag(desc.flags, WindowFlags::Resizable));
        SDL_SetWindowBordered(window, !HasFlag(desc.flags, WindowFlags::Borderless));
        SDL_SetWindowAlwaysOnTop(window, HasFlag(desc.flags, WindowFlags::AlwaysOnTop));
    }

    uint64_t SDLWindow::MapFlags(WindowFlags flags, WindowMode mode) noexcept
    {
        Uint64 sdl_flags = 0;

        if (mode == WindowMode::BorderlessWindowed || mode == WindowMode::ExclusiveFullscreen)
        {
            sdl_flags |= SDL_WINDOW_FULLSCREEN;
        }

        if (HasFlag(flags, WindowFlags::Resizable))      sdl_flags |= SDL_WINDOW_RESIZABLE;
        if (HasFlag(flags, WindowFlags::Borderless))     sdl_flags |= SDL_WINDOW_BORDERLESS;
        if (HasFlag(flags, WindowFlags::HighDPI))        sdl_flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
        if (HasFlag(flags, WindowFlags::AlwaysOnTop))    sdl_flags |= SDL_WINDOW_ALWAYS_ON_TOP;
        if (HasFlag(flags, WindowFlags::StartMaximized)) sdl_flags |= SDL_WINDOW_MAXIMIZED;
        if (HasFlag(flags, WindowFlags::StartMinimized)) sdl_flags |= SDL_WINDOW_MINIMIZED;
        if (HasFlag(flags, WindowFlags::Hidden))         sdl_flags |= SDL_WINDOW_HIDDEN;
        if (HasFlag(flags, WindowFlags::Transparent))    sdl_flags |= SDL_WINDOW_TRANSPARENT;

        return sdl_flags;
    }
	
}