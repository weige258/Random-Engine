#pragma once

namespace RandEngine::Systems { struct System; }

namespace RandEngine::Core::Behaviors{

    struct IFixUpdateBehavior{
        virtual void FixUpdate(float& delta_time,Systems::System &system) = 0;

        virtual ~IFixUpdateBehavior() = default;
    };
}