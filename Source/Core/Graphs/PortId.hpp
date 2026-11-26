#pragma once
#include <concepts>
#include <limits>
#include "NodeId.hpp"

namespace RandomEngine::Core::Graphs
{

    struct PortId final
    {
        friend struct BasePort;

    private:
        NodeId owner_node_id;
        uint32_t index;

    public:
        PortId() = default;
        PortId(NodeId owner, uint32_t index) : owner_node_id(owner), index(index) {}

        ~PortId() = default;

        PortId& operator=(const NodeId &other)
        {
            owner_node_id = other;
            return *this;
        }

        PortId& operator=(const PortId &other)
        {
            owner_node_id = other.owner_node_id;
            index = other.index;
            return *this;
        }


        bool operator==(const PortId &other) const
        {
            return owner_node_id == other.owner_node_id && index == other.index;
        }

        bool operator!=(const PortId &other) const
        {
            return owner_node_id != other.owner_node_id || index != other.index;
        }

        explicit operator bool() const
        {
            return bool(owner_node_id);
        }
    };
}