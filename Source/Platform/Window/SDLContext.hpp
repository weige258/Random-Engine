#pragma once

#include <stdexcept>
#include "memory"
#include "SDL3/SDL.h"

namespace RandEngine::Platform::Window
{

    class SDLContext
    {
    public:
        // 允许传入所需子系统 Flag，默认开启 VIDEO + AUDIO + GAMEPAD
        static std::shared_ptr<SDLContext> GetInstance(SDL_InitFlags flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)
        {
            static std::weak_ptr<SDLContext> instance;
            auto shared = instance.lock();
            if (!shared)
            {
                shared = std::shared_ptr<SDLContext>(new SDLContext(flags));
                instance = shared;
            }
            return shared;
        }

        ~SDLContext()
        {
            SDL_Quit(); // 释放所有已初始化的 SDL 子系统
        }

    private:
        explicit SDLContext(SDL_InitFlags flags)
        {
            if (!SDL_Init(flags))
            {
                throw std::runtime_error("Failed to initialize SDL: " + std::string(SDL_GetError()));
            }
        }
    };

}