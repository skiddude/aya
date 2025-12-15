

#pragma once

#include "Utility/G3DCore.hpp"
#include "Utility/Quaternion.hpp"


namespace Aya
{

class RunningAverageState
{
private:
    Vector3 position;
    Quaternion angles;

    static float weight(); // % of prior average to use

public:
    static int stepsToSleep(); // number of good steps to sleep

    RunningAverageState() {}

    void reset(const CoordinateFrame& cofm);

    void update(const CoordinateFrame& cofm, float radius);

    bool withinTolerance(const CoordinateFrame& cofm, float radius, float tolerance);
};



} // namespace Aya
