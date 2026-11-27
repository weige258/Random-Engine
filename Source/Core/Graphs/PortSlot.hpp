#pragma once
#include <cstdint>
#include <concepts>
#include "BasePort.hpp"

namespace RandomEngine::Core::Graphs
{

    template <std::uint32_t I, BasePortType T>
    struct PortSlot
    {
        static constexpr std::uint32_t kIndex = I;
        using PortType = T;

        T port{}; // 值内嵌，零指针

        BasePort *Get() noexcept
        {
            return std::addressof(port);
        }

        const BasePort *Get() const noexcept
        {
            return std::addressof(port);
        }

        // ★ 关键：隐式转成 unordered_map 的 value_type
        // 不能加 explicit
        operator std::pair<const std::uint32_t, BasePort *>() noexcept
        {
            return {kIndex, Get()};
        }
    };
}