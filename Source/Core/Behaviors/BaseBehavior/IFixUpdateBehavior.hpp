#pragma once

namespace RandomEngine::Systems { struct System; }

namespace RandomEngine::Core::Behaviors{

    struct IFixUpdateBehavior{
        virtual void FixUpdate(float delta_time,Systems::System &system) = 0;

        virtual ~IFixUpdateBehavior() = default;
    };
}