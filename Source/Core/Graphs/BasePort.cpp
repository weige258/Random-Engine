#include "BasePort.hpp"

namespace RandomEngine::Core::Graphs
{
    BasePort::BasePort(const BaseNode &owner) : owner(owner) {
                                                };

    bool BasePort::IsConnected(const BasePort &port) const
    {
        for (const auto &m_port : connected_ports)
        {
            if (&m_port.get() == &port)
            {
                return true;
            }
        }
        return false;
    };

    BaseAnyPort::BaseAnyPort(const BaseNode &owner) : BasePort(owner) {
                                                      };

    PortDirection BaseAnyPort::GetDirection() const
    {
        return PortDirection::Any;
    }

    bool BaseAnyPort::CanConnect(const BasePort &port) const
    {
        if (&owner == &port.owner)
        {
            return false;
        }

        for (const auto &m_port : connected_ports)
        {
            if (&m_port.get() == &port)
            {
                return false;
            }
        }
        return true;
    }

    void BaseAnyPort::Connect(std::variant<
                              std::reference_wrapper<BaseInPort>,
                              std::reference_wrapper<BaseOutPort>,
                              std::reference_wrapper<BaseAnyPort>> &port)
    {
        // variant 本身不能转成 BasePort&，必须先 visit 取出实际端口再校验，
        // 因此 CanConnect 必须写在 lambda 内部。
        std::visit([this](auto &&p)
                   {
        BasePort &other = static_cast<BasePort &>(p.get());

        if (!CanConnect(other))
            return;

        connected_ports.push_back(std::ref(other));
        other.connected_ports.push_back(std::ref(static_cast<BasePort &>(*this))); }, port);
    }

    BaseOutPort::BaseOutPort(const BaseNode &owner) : BasePort(owner) {
                                                      };
    PortDirection BaseOutPort::GetDirection() const
    {
        return PortDirection::Output;
    }

    bool BaseOutPort::CanConnect(const BasePort &port) const
    {
        if (&owner == &port.owner)
        {
            return false;
        }

        for (const auto &m_port : connected_ports)
        {
            if (&m_port.get() == &port)
            {
                return false;
            }
        }

        if ((!dynamic_cast<const BaseInPort *>(&port)) && (!dynamic_cast<const BaseAnyPort *>(&port)))
        {
            return false;
        }
        return true;
    }

    void BaseOutPort::Connect(std::variant<
                              std::reference_wrapper<BaseInPort>,
                              std::reference_wrapper<BaseAnyPort>> &port)
    {
        // 校验必须在 visit 内部：variant 无法转成 BasePort&。
        std::visit([this](auto &&p)
                   {
        BasePort &other = static_cast<BasePort &>(p.get());

        if (!CanConnect(other))
            return;

        connected_ports.push_back(std::ref(other));
        other.connected_ports.push_back(std::ref(static_cast<BasePort &>(*this))); }, port);
    };

    BaseInPort::BaseInPort(const BaseNode &owner) : BasePort(owner) {
                                                    };

    PortDirection BaseInPort::GetDirection() const
    {
        return PortDirection::Input;
    }

    bool BaseInPort::CanConnect(const BasePort &port) const
    {
        if (&owner == &port.owner)
        {
            return false;
        }

        for (const auto &m_port : connected_ports)
        {
            if (&m_port.get() == &port)
            {
                return false;
            }
        }

        if ((!dynamic_cast<const BaseOutPort *>(&port)) && (!dynamic_cast<const BaseAnyPort *>(&port)))
        {
            return false;
        }
        return true;
    }

    void BaseInPort::Connect(std::variant<
                             std::reference_wrapper<BaseOutPort>,
                             std::reference_wrapper<BaseAnyPort>> &port)
    {
        // 同上：校验必须在 visit 内部。
        std::visit([this](auto &&p)
                   {
        BasePort &other = static_cast<BasePort &>(p.get());

        if (!CanConnect(other))
            return;

        connected_ports.push_back(std::ref(other));
        other.connected_ports.push_back(std::ref(static_cast<BasePort &>(*this))); }, port);
    }

}