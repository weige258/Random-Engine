#pragma once
#include <functional>
#include "string"
#include "WindowType.hpp"
#include "WindowDesc.hpp"
#include "memory"

namespace RandEngine::Platform::Window{

class NativeWindow {
public:
    [[nodiscard]] static std::unique_ptr<NativeWindow> Create(const WindowDesc& desc = WindowDesc{});

    virtual ~NativeWindow() = default;

    // 严禁拷贝与移动，多态对象统一使用 std::unique_ptr 管理
    NativeWindow(const NativeWindow&) = delete;
    NativeWindow& operator=(const NativeWindow&) = delete;
    NativeWindow(NativeWindow&&) = delete;
    NativeWindow& operator=(NativeWindow&&) = delete;

    protected:
        NativeWindow() = default;

    public:
        virtual void PollEvents() = 0;
        virtual void SetDesc(const WindowDesc& desc) = 0;

        [[nodiscard]] virtual bool ShouldClose() const = 0;
        [[nodiscard]] virtual const WindowDesc& GetDesc() const = 0;
        [[nodiscard]] virtual void* GetNativeHandle() const = 0;

        // 使用通用 void* 传递原生 OS 事件，或者通过引擎内部的 Event 类解耦
        using NativeEventCallback = std::function<void(void* os_event)>;
        virtual void SetEventCallback(NativeEventCallback cb) = 0;
};
}