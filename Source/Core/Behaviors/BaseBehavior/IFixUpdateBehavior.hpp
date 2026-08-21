#pragma once

namespace RandEngine::Core::Behaviors{

    struct IFixUpdateBehavior{
        virtual void FixUpdate(float delta_time) = 0;

        virtual ~IFixUpdateBehavior() = default;
    };
}