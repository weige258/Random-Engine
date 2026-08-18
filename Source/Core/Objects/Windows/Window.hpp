#pragma once
#include "Objects/BaseObject/BaseObject.hpp"
#include "Behaviors/BaseBehavior/ISystemUpdateBehavior.hpp"
#include "SDL3/SDL.h"
#include "Memory/MasterPtr.hpp"
#include "string"

namespace RandEngine::Core::Objects::Windows
{
    struct Window 
    {
    private:
        SDL_Window* window=nullptr;
        std::string name;
        int width, height;

    public:
        Window(const std::string& name="RandEngine Default Window",int width=600,int height=600);
        ~Window();
        void SystemUpdate();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&& other) noexcept;
        Window& operator=(Window&& other) noexcept;
    };
}