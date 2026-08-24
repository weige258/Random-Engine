#pragma once

namespace RandEngine::Systems{
struct System;

struct ISystem
{
    virtual ~ISystem() = default;

    virtual void Init() = 0;
    virtual void Run(System& system) = 0;
    virtual void Destroy() = 0;
};

}