#include "BasePort.hpp"

namespace RandomEngine::Core::Graphs
{
    BasePort::BasePort(const BaseNode &owner) : owner(owner) {
                                                };

    BasePort::~BasePort()
    {
        DisconnectAll();
    };

    bool BasePort::IsConnected(const BasePort &port) const noexcept
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

    bool BasePort::Disconnect(BasePort &port)
    {
        for (auto it = connected_ports.begin(); it != connected_ports.end(); ++it)
        {
            if (&it->get() == &port)
            {
                connected_ports.erase(it);
                for (auto it2 = port.connected_ports.begin(); it2 != port.connected_ports.end(); ++it2)
                {
                    if (&it2->get() == this)
                    {
                        port.connected_ports.erase(it2);
                        break;
                    }
                }
                return true;
            }
        }
        return false;
    }

    void BasePort::DisconnectAll()
    {
        for (const auto &peer : connected_ports)
        {
            auto &list = peer.get().connected_ports;
            const auto it = std::find_if(list.begin(), list.end(),
                                         [this](const std::reference_wrapper<BasePort> &ref)
                                         { return &ref.get() == this; });
            if (it != list.end())
            {
                list.erase(it);
            }
        }
        connected_ports.clear();
    }

    BaseAnyPort::BaseAnyPort(const BaseNode &owner) : BasePort(owner) {
                                                      };

    PortDirection BaseAnyPort::GetDirection() const noexcept
    {
        return PortDirection::Any;
    }

    bool BaseAnyPort::CanConnect(const BasePort &port) const noexcept
    {
        if (&owner == &port.owner)
        {
            return false;
        }

        return !IsConnected(port) && !port.IsConnected(*this);
    }

    bool BaseAnyPort::Connect(BasePort &port)
    {
        if (!CanConnect(port))
        {
            return false;
        }

        connected_ports.push_back(std::ref(port));
        port.connected_ports.push_back(std::ref(static_cast<BasePort &>(*this)));
        return true;
    }

    void BaseAnyPort::StaticConnect(std::variant<
                                    std::reference_wrapper<BaseInPort>,
                                    std::reference_wrapper<BaseOutPort>,
                                    std::reference_wrapper<BaseAnyPort>> const &port)
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
    PortDirection BaseOutPort::GetDirection() const noexcept
    {
        return PortDirection::Output;
    }

    bool BaseOutPort::CanConnect(const BasePort &port) const noexcept
    {
        if (&owner == &port.owner)
        {
            return false;
        }

        const PortDirection direction = port.GetDirection();
        if (direction != PortDirection::Input && direction != PortDirection::Any)
        {
            return false;
        }

        return !IsConnected(port) && !port.IsConnected(*this);
    }

    bool BaseOutPort::Connect(BasePort &port)
    {
        if (!CanConnect(port))
        {
            return false;
        }

        connected_ports.push_back(std::ref(port));
        port.connected_ports.push_back(std::ref(static_cast<BasePort &>(*this)));
        return true;
    }

    void BaseOutPort::StaticConnect(std::variant<
                                    std::reference_wrapper<BaseInPort>,
                                    std::reference_wrapper<BaseAnyPort>> const &port)
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

    PortDirection BaseInPort::GetDirection() const noexcept
    {
        return PortDirection::Input;
    }

    bool BaseInPort::CanConnect(const BasePort &port) const noexcept
    {
        if (&owner == &port.owner)
        {
            return false;
        }

        const PortDirection direction = port.GetDirection();
        if (direction != PortDirection::Output && direction != PortDirection::Any)
        {
            return false;
        }

        return !IsConnected(port) && !port.IsConnected(*this);
    }

    bool BaseInPort::Connect(BasePort &port)
    {
        if (!CanConnect(port))
        {
            return false;
        }

        connected_ports.push_back(std::ref(port));
        port.connected_ports.push_back(std::ref(static_cast<BasePort &>(*this)));
        return true;
    }

    void BaseInPort::StaticConnect(std::variant<
                                   std::reference_wrapper<BaseOutPort>,
                                   std::reference_wrapper<BaseAnyPort>> const &port)
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