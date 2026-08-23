#pragma once 
#include "Memory/MasterPtr.hpp"
#include "Memory/ObserverPtr.hpp"
#include "Core/Behaviors/BaseBehavior/BaseBehavior.hpp"
#include "Systems/ResourceSystems/BehaviorSystems/BehaviorSystem.hpp"
#include <deque>

namespace RandEngine::Core::Containers {
    struct BehaviorChain {
         private:
           std::deque<Core::Memory::MasterPtr<Core::Behaviors::BaseBehavior>> behaviors_in_chain;
           std::deque<Core::Memory::ObserverPtr<Core::Behaviors::BaseBehavior>> behaviors_in_behavior_system;

        public:
        void UploadBehaviors(Systems::ResourceSystems::BehaviorSystems::BehaviorSystem& behavior_system){
              
        }
    };
}