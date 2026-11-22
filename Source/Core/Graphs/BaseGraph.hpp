#pragma once
#include "BaseNode.hpp"
#include <vector>   
#include "BasePort.hpp"
#include <memory>

namespace RandomEngine::Core::Graphs{

    struct BaseGraph
    {
        protected:
        std::vector<std::reference_wrapper<BaseNode>> nodes;
        std::vector<std::reference_wrapper<BasePort>> ports;

        public:
        BaseGraph();
        virtual ~BaseGraph();
    };
}