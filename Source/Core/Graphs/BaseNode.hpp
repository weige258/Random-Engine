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

        template <BasePortType T>
        bool InsertPort(uint32_t index, T port)
        {
            port.BindNodeId(id);
            port.BindIndex(index);
            for (auto [i, p] : GetAllPorts())
            {
                if (i == index)
                {
                    *p = port;
                    return true;
                }
            }
            return false;
        }

        template <BasePortType T>
        bool InsertPort(T port)
        {
            return InsertPort(port.GetIndex(), port);
        }
    };

    struct TestPort : BasePort
    {
        
        virtual PortDirection GetPortDirection() const override { return PortDirection::Any; }

        virtual PortType GetPortType() const override { return PortType::Static; }

        virtual ~TestPort() = default;
    };

    struct TestNode : BaseNode
    {

        PortSlot<1, TestPort> port;

        virtual std::unordered_map<std::uint32_t, BasePort *> GetAllPorts() override
        {
            return {port};
        }
    };
}