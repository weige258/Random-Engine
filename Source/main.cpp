#include "EngineApplication.hpp"
#include "Core/Objects/BaseObject/BaseObject.hpp"
#include "iostream"
using namespace std;

int main()
{
    RandEngine::Engine::EngineApplication app;
    app.Init();
    app.Run();
    try{
        app.system->resource_system.object_system.Add(RandEngine::Core::Objects::BaseObject());
        app.system->resource_system.object_system.Add(RandEngine::Core::Objects::BaseObject());
        
        for(auto& i:app.system->resource_system.object_system.GetAll()){
            cout<<i.id<<endl;
            i.id+=100;
        }

        for(RandEngine::Core::Objects::BaseObject& i:app.system->resource_system.object_system.GetAll()){
            cout<<i.id<<endl;
            i.id+=100;
        }
    }catch(...){cout<<"Error"<<endl;}
    
    app.Destroy();
    return 0;
}