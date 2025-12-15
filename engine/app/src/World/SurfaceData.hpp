

#include "World/Controller.hpp"

#pragma once

namespace Aya
{

class SurfaceData
{
public:
    LegacyController::InputType inputType;
    float paramA;
    float paramB;

    SurfaceData()
        : inputType(LegacyController::NO_INPUT)
        , paramA(-0.5)
        , paramB(0.5)
    {
    }

    bool operator==(const SurfaceData& other) const
    {
        return (inputType == other.inputType && paramA == other.paramA && paramB == other.paramB);
    }

    static const SurfaceData& empty()
    {
        static SurfaceData s;
        return s;
    }

    bool isEmpty() const
    {
        return *this == empty();
    }
};

} // namespace Aya