#pragma once

#include "Core/Behaviors/BaseBehavior/BaseBehavior.hpp"
#include <memory>
#include<vector>
namespace RandEngine::Systems::BehaviorSystems{

class BehaviorSystem {
    private:
    std::vector<std::shared_ptr<Core::Behaviors::BaseBehavior>> all_behaviors;

    public:
};

}