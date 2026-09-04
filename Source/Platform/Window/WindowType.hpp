#pragma once

#include <stdint.h>
#include <string>

namespace RandomEngine::Platform::Window
{
    //窗体类型
    enum class WindowMode : uint8_t
    {
        Windowed,           // 1. 普通窗口
        BorderlessWindowed, // 2. 无边框全屏 / 窗口化全屏
        ExclusiveFullscreen // 3. 独占全屏
    };

    //窗体参数
    enum class WindowFlags : uint32_t
    {
        None = 0,
        Resizable = 1 << 0,      // 是否允许拖拽改变大小
        Movable = 1 << 1,        // 是否允许鼠标拖拽移动
        Borderless = 1 << 2,     // 是否隐藏标题栏和边框
        AlwaysOnTop = 1 << 3,    // 是否保持窗口置顶
        HighDPI = 1 << 4,        // 是否开启 High DPI / Retina 支持
        StartMaximized = 1 << 5, // 启动时默认最大化
        StartMinimized = 1 << 6, // 启动时默认最小化
        Hidden = 1 << 7,         // 初始隐藏
        Transparent = 1 << 8,    // 背景透明
        FocusOnShow = 1 << 9     // 显示时自动强行获取焦点
    };

  
    constexpr WindowFlags operator|(WindowFlags a, WindowFlags b) noexcept
    {
        return static_cast<WindowFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    constexpr WindowFlags operator&(WindowFlags a, WindowFlags b) noexcept
    {
        return static_cast<WindowFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    constexpr WindowFlags &operator|=(WindowFlags &a, WindowFlags b) noexcept
    {
        a = a | b;
        return a;
    }

    constexpr bool HasFlag(WindowFlags flags, WindowFlags flag) noexcept
    {
        return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
    }

}