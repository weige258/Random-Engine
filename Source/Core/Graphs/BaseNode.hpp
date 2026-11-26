#pragma once
#include "NodeId.hpp"
#include "BasePort.hpp"
#include <vector>
#include "PortId.hpp"
#include "PortSlot.hpp"
#include <unordered_map>
namespace RandomEngine::Core::Graphs
{
    struct BaseNode
    {
    protected:
        NodeId id;

    public:
        BaseNode(NodeId id) : id(id) {}

        virtual ~BaseNode() = default;

        virtual std::unordered_map<std::uint32_t, BasePort *> GetAllPorts() = 0;

    private:
    };

    struct TestPort : BasePort
    {
        virtual PortDirection GetPortDirection() override { return PortDirection::Any; }

        virtual PortType GetPortType() override { return PortType::Static; }

        virtual ~TestPort() = default;
    };

    struct TestNode : BaseNode
    {
        
        PortSlot<1, TestPort> port;
         
        virtual std::unordered_map<std::uint32_t, BasePort *> GetAllPorts() override{
                return {port};
        }
    };
}