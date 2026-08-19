#include "WindowManager.hpp"
#include <stdexcept>
#include "Windows/SDLWindow.hpp"
#include "SDL3/SDL.h"

namespace RandEngine::Platform::Manager{
    void WindowManager::Init(){
        if(!SDL_Init(SDL_INIT_VIDEO)){
            throw std::runtime_error("Failed to initialize SDL: " + std::string(SDL_GetError()));
        }

        windows.push_back(std::make_unique<Windows::SDLWindow>(Windows::SDLWindow(Windows::WindowDesc())));
    }

    void WindowManager::Run(){
        for(auto& window : windows){
            window->PollEvents();
        }
    }

    void WindowManager::Destroy(){
        windows.clear();
    }
}