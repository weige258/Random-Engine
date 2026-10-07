#include "BaseJob.hpp"
#include "Config.hpp"
#include "Behaviors/BaseBehavior/BaseBehavior.hpp"
#include "Behaviors/BaseBehavior/IFixUpdateBehavior.hpp"

namespace RandomEngine::Core::Jobs::Job
{

    using FixUpdateSlot = Core::Jobs::Job::MethodSlot<
        &Behaviors::IFixUpdateBehavior::FixUpdate,
        Config::TimeType,
        ::RandomEngine::Systems::System &>;
    using FixUpdateBehaviorJob = Core::Jobs::Job::BaseJob<FixUpdateSlot>;

}