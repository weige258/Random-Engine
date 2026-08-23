#include "NativeWindow.hpp"
#include "WindowDesc.hpp"
#include "SDLWindow.hpp"

#if defined(_WIN32)
    // #include "Win32Window.hpp"
#elif defined(__APPLE__)
    // #include "CocoaWindow.hpp"
#endif

namespace RandEngine::Platform::Window {

std::unique_ptr<NativeWindow> NativeWindow::Create(const WindowDesc& desc)
{
#if defined(_WIN32)
    return std::make_unique<SDLWindow>(desc); 
#elif defined(__APPLE__)
    return std::make_unique<SDLWindow>(desc); 
#else
    return std::make_unique<SDLWindow>(desc);
#endif
}

}