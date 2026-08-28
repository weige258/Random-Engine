#include "BaseObject.hpp"

namespace RandEngine::Core::Objects
{
    BaseObject::BaseObject() = default;
    BaseObject::~BaseObject() = default;
    
    std::vector<Memory::MasterPtr<Behaviors::BindBaseBehavior>> BaseObject::GetBehaviors(){
           return {};
    };
}