#include "ObjectSystem.hpp"
#include "System.hpp"
#include "iostream"
namespace RandomEngine::Systems::ResourceSystems
{

    void ObjectSystem::InitLockSystem(uint32_t cpu_l3_cache_kb)
    {
        const uint32_t cpu_l3_cache_bytes = cpu_l3_cache_kb * 1024;
        uint32_t lock_size = (cpu_l3_cache_bytes / sizeof(Core::Memory::IdLock))/8;
        if(lock_size<4096){lock_size=4096;}
        m_locks.resize(lock_size);
    }

    // 系统执行
    void ObjectSystem::Init(System &system)
    {
        InitLockSystem(system.device_system.cpu_system.GetCPUInfo().l3_cache_size);
    }

    void ObjectSystem::Run(System &system)
    {
        
    }

    void ObjectSystem::Destroy()
    {
    }

}