#include "LogicUpdateSystem.hpp"
#include "Systems/System.hpp"


namespace RandEngine::Systems::RuntimeSystems{
    
    void LogicUpdateSystem::UploadLogicBehaviorsFromBehaviorSystem(System& system)
    {
        // 直接接收 vector<ObserverPtr<ILogicUpdateBehavior>>
        auto [pending_adds, pending_deletes] = 
            system.resource_system.behavior_system.RemoveBehaviorToRuntimeSystem<Core::Behaviors::ILogicUpdateBehavior>();

        if (pending_adds.empty() && pending_deletes.empty())
            return;

        // 移除失效 Task
        for (const auto& logic_ptr : pending_deletes)
        {
            if (logic_ptr)
            {
                logic_behavior_job_worker.RemoveTask(logic_ptr);
            }
        }

        // 压入新增 Task
        for (const auto& logic_ptr : pending_adds)
        {
            if (logic_ptr)
            {
                logic_behavior_job_worker.PushTask(logic_ptr);
            }
        }
    }


    void LogicUpdateSystem::Init(System &system){
        logic_behavior_job_worker.SetSystem(system);
        
        uint32_t require_thread_count = (system.device_system.cpu_system.GetCPUInfo().logical_processor_count/4);
        logic_behavior_job_worker.Start(require_thread_count);
    }

    void LogicUpdateSystem::Run(System &system){
        count++;
        if (update_rate > 0 && (count % update_rate == 0)){
            UploadLogicBehaviorsFromBehaviorSystem(system);
        }
    }

    void LogicUpdateSystem::Destroy(){
        
    }
}