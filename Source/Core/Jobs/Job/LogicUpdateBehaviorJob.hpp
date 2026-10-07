#include "BaseJob.hpp"
#include "Core/Config.hpp"
#include "Behaviors/BaseBehavior/BaseBehavior.hpp"
#include "Behaviors/BaseBehavior/ILogicUpdateBehavior.hpp"

namespace RandomEngine::Core::Jobs::Job{

    using LogicUpdateBehaviorSlot = Core::Jobs::Job::MethodSlot<&Behaviors::ILogicUpdateBehavior::LogicUpdate,
                                                         Config::TimeType,
                                                         ::RandomEngine::Systems::System &>;
    using LogicUpdateBehaviorJob = Core::Jobs::Job::BaseJob<LogicUpdateBehaviorSlot>;

}