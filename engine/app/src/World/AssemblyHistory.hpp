

#pragma once

#include "Utility/PhysicsCoord.hpp"
#include "Utility/Average.hpp"
#include "Debug.hpp"

namespace Aya
{

class Assembly;

class AssemblyHistory
{
private:
    Average<PhysicsCoord> average;
    int stepsSinceSample;
    int awakeSteps;
    float maxDeviationSquared;

    static size_t sampleSkip();
    static size_t bufferSize();
    static float sleepTolerance();
    static float sleepToleranceSquared();

    bool notMoving();
    void updateMaxDeviationSquared();
    PhysicsCoord getAssemblyPhysicsCoord(Assembly& a);

public:
    AssemblyHistory(Assembly& a);

    ~AssemblyHistory();

    void clear(Assembly& a);

    bool sampleAndNotMoving(Assembly& a);

    bool preventNeighborSleep();

    void wakeUp();
};

} // namespace Aya
