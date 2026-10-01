#pragma once
#include "Core/Config.hpp"
#include <compare>
#include <cstddef>
#include <functional>

namespace RandomEngine::Core::Objects
{
    struct ObjectId final
    {
        Core::Config::ObjectIDType id = 0;

        constexpr ObjectId() = default;
        constexpr ObjectId(Core::Config::ObjectIDType v) : id(v) {}

        constexpr operator Core::Config::ObjectIDType() const noexcept
        {
            return id;
        }

        constexpr bool operator==(const ObjectId &) const noexcept = default;
        constexpr auto operator<=>(const ObjectId &) const noexcept = default;
    };
}

template <>
struct std::hash<RandomEngine::Core::Objects::ObjectId>
{
    size_t operator()(const RandomEngine::Core::Objects::ObjectId &k) const noexcept
    {
        return std::hash<RandomEngine::Core::Config::ObjectIDType>{}(k.id);
    }
};