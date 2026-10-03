#pragma once
#include <tuple>
#include <type_traits>
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

    template <auto Method, typename... ExecuteArgs>
    struct MethodSlot
    {
        using InterfaceType = typename MethodClassOf<Method>::type;
        using ExecuteArgsTuple = std::tuple<ExecuteArgs...>;
        static constexpr auto MethodPtr = Method;
    };

    namespace Detail
    {
        template <typename... Slots>
        struct CommonInterfaceType;

        template <typename First>
        struct CommonInterfaceType<First>
        {
            using type = typename First::InterfaceType;
        };

        template <typename First, typename Second, typename... Rest>
        struct CommonInterfaceType<First, Second, Rest...>
        {
            using type = typename First::InterfaceType;
            static_assert(std::is_same_v<typename Second::InterfaceType, type>,
                          "All MethodSlots must share the same InterfaceType");
        };

        template <typename... Slots>
        using CommonInterfaceType_t = typename CommonInterfaceType<Slots...>::type;

        template <typename Slot, typename... Slots>
        inline constexpr bool IsSlotOf_v = (std::is_same_v<Slot, Slots> || ...);

        template <typename Slot, typename... Args>
        concept SlotCallable = requires(typename Slot::InterfaceType * p, Args &&...args)
        {
            (p->*Slot::MethodPtr)(std::forward<Args>(args)...);
        };
    }

    template <typename... Slots>
    struct BaseJob
    {
    public:
        using InterfaceType = Detail::CommonInterfaceType_t<Slots...>;
        using SlotTuple = std::tuple<Slots...>;
        using FirstSlot = std::tuple_element_t<0, SlotTuple>;
        using ExecuteArgsTuple = typename FirstSlot::ExecuteArgsTuple;

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

        template <typename SlotType, typename... Args>
            requires(Detail::IsSlotOf_v<SlotType, Slots...> &&
                     Detail::SlotCallable<SlotType, Args...>)
        void Execute(Args &&...args) const
        {
            if (m_behavior.Lock())
            {
                (m_behavior.Get()->*SlotType::MethodPtr)(std::forward<Args>(args)...);
            }
        }

        template <typename... Args>
            requires(sizeof...(Slots) == 1 &&
                     Detail::SlotCallable<std::tuple_element_t<0, SlotTuple>, Args...>)
        void Execute(Args &&...args) const
        {
            Execute<std::tuple_element_t<0, SlotTuple>>(std::forward<Args>(args)...);
        }

        explicit operator bool() const { return static_cast<bool>(m_behavior); }
        bool operator==(const BaseJob &other) const { return m_behavior == other.m_behavior; }
        bool operator!=(const BaseJob &other) const { return m_behavior != other.m_behavior; }
        bool operator==(const std::nullptr_t &) const { return m_behavior == nullptr; }
        bool operator!=(const std::nullptr_t &) const { return m_behavior == nullptr; }

        Memory::ObserverPtr<InterfaceType> GetBehavior() const { return m_behavior; }
    };

    template <typename T>
    inline constexpr bool IsBaseJob_v = false;

    template <typename... Slots>
    inline constexpr bool IsBaseJob_v<BaseJob<Slots...>> = true;

    template <typename T>
    concept IsBaseJob = IsBaseJob_v<std::remove_cvref_t<T>>;
}