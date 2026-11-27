#pragma once
#include "PortId.hpp"
#include <tuple>

namespace RandomEngine::Core::Graphs
{

    enum PortDirection
    {
        Input,
        Output,
        Any
    };

    enum PortType
    {
        Static,
        Dynamic
    };


    struct BasePort
    {
    protected:
        PortId port_id;

    public:
        BasePort() = default;

        explicit BasePort(PortId port_id) : port_id(port_id) {}

        explicit BasePort(NodeId owner, uint32_t index) : port_id(owner, index) {}

        virtual ~BasePort() {};

        void BindPortId(PortId port_id) { port_id = port_id; }

        void BindNodeId(NodeId node_id) { port_id.owner_node_id = node_id; }

        void BindIndex(uint32_t index) { port_id.index = index; }

        NodeId GetNodeId() const { return port_id.owner_node_id; }

        PortId GetPortId() const { return port_id; }

        uint32_t GetIndex() const { return port_id.index; }

        virtual PortDirection GetPortDirection() const = 0;

        virtual PortType GetPortType() const = 0;
    };


    template <class T>
    concept BasePortType =
        std::derived_from<T, BasePort> &&
        !std::is_abstract_v<T> &&
        std::default_initializable<T>;

}