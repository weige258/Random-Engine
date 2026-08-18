#include "Window.hpp"
#include "SDL3/SDL.h"
#include "iostream"

namespace RandEngine::Core::Objects::Windows{
    Window::Window(const std::string& name,int width,int height){
        this->name=name;
        this->width=width;
        this->height=height;

        window=SDL_CreateWindow(name.c_str(),width,height,0);
    }

    Window::~Window(){
        if (window) {
            SDL_DestroyWindow(window);
            window = nullptr;
        }
    }

    Window::Window(Window&& other) noexcept 
        : window(other.window), name(std::move(other.name)), width(other.width), height(other.height) {
        other.window = nullptr; 
    }

    Window& Window::operator=(Window&& other) noexcept {
        if (this != &other) {
            if (window) SDL_DestroyWindow(window);
            window = other.window;
            name = std::move(other.name);
            width = other.width;
            height = other.height;
            other.window = nullptr;
        }
        return *this;
    }
}