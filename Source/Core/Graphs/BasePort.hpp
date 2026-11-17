#pragma once

#include "BaseNode.hpp"
#include <vector>
#include <functional>
#include <variant>
#include <algorithm>

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

    // 会话视图契约（进程内，非持久层）：
    //   1. 生命周期：端口 ⊂ 节点 ⊂ 图；端口不进容器、不搬移、不存盘（赋值已隐式删除）。
    //   2. 销毁顺序：先摘边（Disconnect / 清空 connected_ports），再删节点，否则连接表悬挂。
    //   3. 需要持久化 / remap / 跨 Chunk 的边一律去上层用可重映射 ID 表达，基类不参与。
    struct BasePort
    {

        friend struct BaseInPort;
        friend struct BaseOutPort;
        friend struct BaseAnyPort;

    protected:
        const BaseNode &owner;
        std::vector<std::reference_wrapper<BasePort>> connected_ports;

    public:
        explicit BasePort(const BaseNode &owner);

        virtual PortDirection GetDirection() const noexcept = 0;

        virtual bool CanConnect(const BasePort &port) const noexcept = 0;

        bool IsConnected(const BasePort &port) const noexcept;

        virtual bool Connect(BasePort &port) = 0;

        bool Disconnect(BasePort& port);

        void DisconnectAll();

        virtual ~BasePort() ;
    };

    struct BaseAnyPort : BasePort
    {

        BaseAnyPort(const BaseNode &owner);

        PortDirection GetDirection() const noexcept override;

        bool CanConnect(const BasePort &port) const noexcept override;

        virtual bool Connect(BasePort &port) override;
        
        void StaticConnect(std::variant<
                     std::reference_wrapper<BaseInPort>,
                     std::reference_wrapper<BaseOutPort>,
                     std::reference_wrapper<BaseAnyPort>> const &port);
    };

    struct BaseOutPort : BasePort
    {

        BaseOutPort(const BaseNode &owner);

        PortDirection GetDirection() const noexcept override;

        bool CanConnect(const BasePort &port) const noexcept override;

        virtual bool Connect(BasePort &port) override;

        void StaticConnect(std::variant<
                     std::reference_wrapper<BaseInPort>,
                     std::reference_wrapper<BaseAnyPort>> const &port);
    };

    struct BaseInPort : BasePort
    {

        BaseInPort(const BaseNode &owner);

        PortDirection GetDirection() const noexcept override;

        bool CanConnect(const BasePort &port) const noexcept override;

        virtual bool Connect(BasePort &port) override;

        void StaticConnect(std::variant<
                     std::reference_wrapper<BaseOutPort>,
                     std::reference_wrapper<BaseAnyPort>> const &port);
    };
}