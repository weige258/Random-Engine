#pragma once

#include "string"
#include "WindowType.hpp"

namespace RandEngine::Platform::Window
{
    // 窗体信息 WindowDesc 结构体
    struct WindowDesc
    {
        std::string title = "RandEngine Window";
        int width = 1280;
        int height = 720;
        WindowMode mode = WindowMode::Windowed;
        WindowFlags flags = WindowFlags::Resizable | WindowFlags::HighDPI | WindowFlags::Movable;
    };
}