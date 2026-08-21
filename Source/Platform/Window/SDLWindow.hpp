#pragma once
#include <memory>
#include "SDLContext.hpp"
#include "NativeWindow.hpp"
#include "WindowType.hpp"
#include "WindowDesc.hpp"
#include "SDL3/SDL.h"
#include <string>
#include <stdexcept>

namespace RandEngine::Platform::Window
{
    class SDLWindow final : public NativeWindow
    {
    private:
        std::shared_ptr<SDLContext> m_context;
        SDL_Window *window = nullptr;
        WindowDesc desc{};
        bool should_close = false;
        NativeEventCallback event_callback = nullptr;

    public:
        explicit SDLWindow(const WindowDesc &desc);
        ~SDLWindow() override;

        // 禁用拷贝语义（保证原生句柄生命周期唯一）
        SDLWindow(const SDLWindow &) = delete;
        SDLWindow &operator=(const SDLWindow &) = delete;

        // 移动语义
        SDLWindow(SDLWindow &&other) noexcept;
        SDLWindow &operator=(SDLWindow &&other) noexcept;

        // IWindow 接口实现
        void PollEvents() override;
        void SetDesc(const WindowDesc &desc) override;

        [[nodiscard]] bool ShouldClose() const override { return should_close; }
        [[nodiscard]] const WindowDesc &GetDesc() const override { return desc; }
        [[nodiscard]] void *GetNativeHandle() const override { return window; }

        void SetEventCallback(NativeEventCallback cb) override { event_callback = std::move(cb); }

    private:
        static uint64_t MapFlags(WindowFlags flags, WindowMode mode) noexcept;
    };
}
