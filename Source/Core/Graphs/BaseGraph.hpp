#pragma once

#include "BaseNode.hpp"
#include "BasePort.hpp"
#include "BaseEdge.hpp"
#include "Containers/GenerationalSlotArray.hpp"
#include "Memory/MasterPtr.hpp"
#include <utility>
#include <vector>
#include <memory>

namespace RandomEngine::Core::Graphs
{
    struct BaseGraph
    {

    protected:
        Containers::GenerationalSlotArray<Memory::MasterPtr<BaseNode>,
                                          decltype(std::declval<NodeId>().id),
                                          decltype(std::declval<NodeId>().generation)>
            nodes;

        std::vector<BaseEdge> edges;

    public:
        
        BaseGraph() = default;

        virtual ~BaseGraph(){};
        
        bool IsConnected(PortId from, PortId to) const{
            for(const auto& edge : edges){
                if((edge.From() == from && edge.To() == to)||((edge.From() == to && edge.To() == from))){
                    return true;
                }
            }
            return false;
        };

        bool CanDirectionConnect(const PortId& from,const PortId& to) const{
            PortDirection from_direction = PortDirection::Any;
            PortDirection to_direction = PortDirection::Any;

            nodes.ForEach([&](const auto&, const auto& node){
                if(!node){
                    return;
                }

                for(const auto& [_, port] : node->GetAllPorts()){
                    const PortId port_id = port->GetPortId();

                    if(port_id == from){
                        from_direction = port->GetPortDirection();
                    }

                    if(port_id == to){
                        to_direction = port->GetPortDirection();
                    }
                }
            });

            if(from_direction == PortDirection::Any || to_direction == PortDirection::Any){
                return true;
            }

            return (from_direction == PortDirection::Input && to_direction == PortDirection::Output) ||
                   (from_direction == PortDirection::Output && to_direction == PortDirection::Input);
        }

        bool BaseCanConnect(const PortId& from,const PortId& to) const{
            
            //有效性检查
            if(!bool(from) || !bool(to)){ return false; }
            //不能连接到自己
            if(from == to){ return false; }
            //不能连接到已连接的端口
            if(IsConnected(from,to)){
                return false;
            }
            //不能连接到不同方向的端口
            if(!CanDirectionConnect(from,to)){
                return false;
            }

            return true;
        }

        virtual bool CanConnect(const PortId& from,const PortId& to) {
            return BaseCanConnect(from,to);
        };

        bool BaseConnect(const PortId& from,const PortId& to){
            if(!BaseCanConnect(from,to)){
                return false;
            }
            edges.push_back(BaseEdge(from,to));
            return true;
        }

        virtual bool Connect(const PortId& from,const PortId& to) {
            if(CanConnect(from,to)){
                edges.push_back(BaseEdge(from,to));
                return true;
            }
            return false;
        };

        bool Disconnect(const PortId& from,const PortId& to){
            if(!IsConnected(from,to)){
                return false;
            }
            edges.erase(std::remove_if(edges.begin(), edges.end(), [from, to](const BaseEdge& edge){
                return (edge.From() == from && edge.To() == to) ||
                       (edge.From() == to && edge.To() == from);
            }), edges.end());
            return true;
        }

        void DisconnectAll(){
            edges.clear();
        }
    };
}