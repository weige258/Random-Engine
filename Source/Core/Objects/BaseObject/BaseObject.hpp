#pragma once
#include <cstdint>

namespace RandEngine::Core::Objects {
    struct BaseObject { 
        int64_t id;
              
        BaseObject();

        virtual ~BaseObject();
    };
}