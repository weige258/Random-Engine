#pragma once
#include "PortId.hpp"
#include "BasePort.hpp"

namespace RandomEngine::Core::Graphs
{

    struct BaseEdge
    {
    protected:
        PortId from;
        PortId to;

    public:
        BaseEdge() = default;

        BaseEdge(PortId from, PortId to) : from(from), to(to) {}

        virtual ~BaseEdge() {};

        explicit operator bool() const
        {
            if (bool(from) && bool(to))
            {
                return true;
            }
            return false;
        }

        const PortId &From() const { return from; }
        const PortId &To() const { return to; }
    };
}