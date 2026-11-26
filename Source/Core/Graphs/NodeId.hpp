#pragma once
#include <cstdint>
#include "Core/Containers/GenerationalSlotArray.hpp"
namespace RandomEngine::Core::Graphs
{
    struct NodeId
    {
        uint64_t id=Containers::SlotId<decltype(id), uint64_t>::kInvalidIndex;
        uint64_t generation=Containers::SlotId<uint64_t, decltype(generation)>::kInvalidGeneration;
        
        NodeId() = default;

        NodeId(uint64_t id, uint64_t generation) : id(id), generation(generation) {}

        NodeId(Containers::SlotId<decltype(id), decltype(generation)> slot_id) : id(slot_id.index), generation(slot_id.generation) {}
        
        template <typename T, typename U>
        operator Containers::SlotId<T, U>() const
        {
            return Containers::SlotId<T, U>(id, generation);
        }

        explicit operator bool() const
        {
            return id != Containers::SlotId<decltype(id), uint64_t>::kInvalidIndex &&
                   generation != Containers::SlotId<decltype(generation), uint64_t>::kInvalidGeneration;
        }

        friend bool operator==(const NodeId &a, const NodeId &b) noexcept
        {
            return a.id == b.id && a.generation == b.generation;
        }

        friend bool operator!=(const NodeId &a, const NodeId &b) noexcept { 
            return !(a == b); 
        }
    };
}