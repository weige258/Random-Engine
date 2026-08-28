#include "EngineMain.hpp"
#include "Core/Objects/BaseObject/BaseObject.hpp"
#include "Core/Behaviors/BaseBehavior/BindBaseBehavoir.hpp"
#include "iostream"

struct EnityCall : public RandEngine::Core::Behaviors::BindBaseBehavior, public RandEngine::Core::Behaviors::ILogicUpdateBehavior
{
    void LogicUpdate(float delta_time, RandEngine::Systems::System &system) override
    {
        std::cout << this->bind_id << std::endl;
    }
    
};

struct Enity : public RandEngine::Core::Objects::BaseObject
{
    BIND_BEHAVIORS(EnityCall);
};

int main()
{

    RandEngine::Engine::EngineMain app;
    app.Init();
    app.system->resource_system.Add(Enity());
    app.Run();
    app.Destroy();
    return 0;
}