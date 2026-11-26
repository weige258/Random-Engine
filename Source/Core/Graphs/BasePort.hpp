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

        virtual PortDirection GetPortDirection() = 0;

        virtual PortType GetPortType() = 0;
    };

    struct BaseAnyPort : public BasePort
    {

        BaseAnyPort(PortId port_id) : BasePort(port_id) {}
        virtual ~BaseAnyPort() {};

        virtual PortDirection GetPortDirection() override { return PortDirection::Any; }
    };

    struct BaseInPort : public BasePort
    {

        BaseInPort(PortId port_id) : BasePort(port_id) {}
        virtual ~BaseInPort() {};

        virtual PortDirection GetPortDirection() override { return PortDirection::Input; }
    };

    struct BaseOutPort : public BasePort
    {

        BaseOutPort(PortId port_id) : BasePort(port_id) {}
        virtual ~BaseOutPort() {};

        virtual PortDirection GetPortDirection() override { return PortDirection::Output; }
    };

}