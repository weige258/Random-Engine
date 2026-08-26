#include "BindBaseBehavior.hpp"

namespace RandEngine::Core::Behaviors{
    BindBaseBehavior::BindBaseBehavior()=default;
    BindBaseBehavior::BindBaseBehavior(Config::ObjectIDType bind_id) : bind_id(bind_id) {}
    BindBaseBehavior::~BindBaseBehavior()=default;

}