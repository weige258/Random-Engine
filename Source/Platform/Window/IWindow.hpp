#pragma once
#include <functional>
#include "string"
#include "WindowType.hpp"
#include "WindowDesc.hpp"

namespace RandEngine::Platform::Window{

class IWindow {
public:
    virtual ~IWindow() = default;

    // 严禁拷贝与移动，多态对象统一使用 std::unique_ptr 管理
    IWindow(const IWindow&) = delete;
    IWindow& operator=(const IWindow&) = delete;
    IWindow(IWindow&&) = delete;
    IWindow& operator=(IWindow&&) = delete;

    protected:
        IWindow() = default;

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