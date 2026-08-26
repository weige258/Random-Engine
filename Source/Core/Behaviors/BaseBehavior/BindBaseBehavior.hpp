#pragma once
#include "BaseBehavior.hpp"
#include "Core/Config.hpp"

namespace RandEngine::Core::Behaviors{

    struct BindBaseBehavior : public BaseBehavior
    {
        Config::ObjectIDType bind_id;

        BindBaseBehavior();
        BindBaseBehavior(Config::ObjectIDType bind_id);
        ~BindBaseBehavior();
    };
}