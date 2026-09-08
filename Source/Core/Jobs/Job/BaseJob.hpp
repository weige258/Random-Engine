#pragma once
#include <tuple>
#include "Core/Memory/MasterPtr.hpp"
#include "Core/Memory/ObserverPtr.hpp"

namespace RandomEngine::Core::Jobs::Job
{
    template <auto Method>
    struct MethodClassOf;

    template <typename Class, typename Ret, typename... Args, Ret (Class::*method)(Args...)>
    struct MethodClassOf<method>
    {
        using type = Class;
    };

    template <auto Method, typename... ExcuteArgs>
    struct BaseJob
    {
    public:
        using InterfaceType = typename MethodClassOf<Method>::type;
        using ExecuteArgsTuple = std::tuple<ExcuteArgs...>;

    private:
        Memory::ObserverPtr<InterfaceType> m_behavior = nullptr;

    public:
        BaseJob() = default;

        template <typename ConcreteBehavior>
        BaseJob(Memory::MasterPtr<ConcreteBehavior> &behavior)
            : m_behavior(behavior) {}

        template <typename ConcreteBehavior>
        BaseJob(Memory::ObserverPtr<ConcreteBehavior> behavior)
            : m_behavior(behavior) {}

        ~BaseJob() = default;

        void Execute(ExcuteArgs... args) const
        {
            if (m_behavior)
            {
                (m_behavior.Get()->*Method)(args...);
            }
        }

        explicit operator bool() const { return static_cast<bool>(m_behavior); }
        bool operator==(const BaseJob &other) const { return m_behavior == other.m_behavior; }
        bool operator!=(const BaseJob &other) const { return m_behavior != other.m_behavior; }
        bool operator==(const std::nullptr_t &) const { return m_behavior == nullptr; }
        bool operator!=(const std::nullptr_t &) const { return m_behavior != nullptr; }

        Memory::ObserverPtr<InterfaceType> GetBehavior() const { return m_behavior; }
    };

    // job模板特化条件

    template <typename T>
    inline constexpr bool IsBaseJob_v = false;

    template <auto Method, typename... Args>
    inline constexpr bool IsBaseJob_v<BaseJob<Method, Args...>> = true;

    template <typename T>
    concept IsBaseJob = IsBaseJob_v<std::remove_cvref_t<T>>;
}