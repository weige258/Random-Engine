#pragma once

#include "Core/Behaviors/BaseBehavior/BaseBehavior.hpp"
#include <memory>
#include<vector>
namespace RandEngine::Core::Systems::BehaviorSystems{

class BehaviorSystem {
    private:
    std::vector<std::shared_ptr<Behaviors::BaseBehavior>> all_behaviors;

    public:
};

}