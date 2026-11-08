#pragma once

#include "BaseNode.hpp"
#include <vector>
#include <functional>
#include <variant>

namespace RandomEngine::Core::Graphs
{
    struct BaseAnyPort;
    struct BaseInPort;
    struct BaseOutPort;

    enum class PortDirection
    {
        Input,
        Output,
        Any
    };

    struct BasePort
    {

        friend struct BaseInPort;
        friend struct BaseOutPort;
        friend struct BaseAnyPort;

    protected:
        const BaseNode &owner;
        std::vector<std::reference_wrapper<BasePort>> connected_ports;

    public:
        BasePort(const BaseNode &owner);

        virtual PortDirection GetDirection() const = 0;

        virtual bool CanConnect(const BasePort &port) const=0;

        bool IsConnected(const BasePort& port) const;

        virtual ~BasePort() = default;
    };

    struct BaseAnyPort : BasePort
    {

        BaseAnyPort(const BaseNode &owner);

        PortDirection GetDirection() const override;

        bool CanConnect(const BasePort &port) const override;

        void Connect(std::variant<
                     std::reference_wrapper<BaseInPort>,
                     std::reference_wrapper<BaseOutPort>,
                     std::reference_wrapper<BaseAnyPort>> &port);
    };

    struct BaseOutPort : BasePort
    {

        BaseOutPort(const BaseNode &owner);

        PortDirection GetDirection() const override;

        bool CanConnect(const BasePort &port) const override;

        void Connect(std::variant<
                     std::reference_wrapper<BaseInPort>,
                     std::reference_wrapper<BaseAnyPort>> &port);
    };

    struct BaseInPort : BasePort
    {

        BaseInPort(const BaseNode &owner);

        PortDirection GetDirection() const override;

        bool CanConnect(const BasePort &port) const override;

        void Connect(std::variant<
                     std::reference_wrapper<BaseOutPort>,
                     std::reference_wrapper<BaseAnyPort>> &port);
    };
}