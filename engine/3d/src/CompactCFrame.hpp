#pragma once

#include "Utility/Math.hpp"


namespace Aya
{
class CompactCFrame
{
    Vector3 rotationaxis;
    float rotationangle; // angle is always positive.
public:
    Vector3 translation;

    CompactCFrame()
        : rotationangle(0)
    {
    }
    CompactCFrame(const CoordinateFrame& cframe)
        : translation(cframe.translation)
    {
        cframe.rotation.toAxisAngle(rotationaxis, rotationangle);
        AYAASSERT(!Math::hasNanOrInf(rotationaxis));
        AYAASSERT(!Math::isNanInf(rotationangle));
    }

    CompactCFrame(const Vector3& translation, const Vector3& axisAngle)
        : translation(translation)
        , rotationaxis(axisAngle)
    {
        rotationangle = rotationaxis.unitize();
        AYAASSERT(!Math::hasNanOrInf(rotationaxis));
        AYAASSERT(!Math::isNanInf(rotationangle));
    }

    CompactCFrame(const Vector3& translation, const Vector3& axis, float angle)
        : translation(translation)
    {
        setAxisAngle(axis, angle);
        AYAASSERT(!Math::hasNanOrInf(rotationaxis));
        AYAASSERT(!Math::isNanInf(rotationangle));
    }

    void setAxisAngle(const Vector3& axis, float angle)
    {
        // enforce angle > 0.
        if (angle >= 0)
        {
            rotationaxis = axis;
            rotationangle = angle;
        }
        else
        {
            rotationaxis = axis * -1.0f;
            rotationangle = -angle;
        }
    }

    CoordinateFrame getCFrame() const
    {
        CoordinateFrame answer(Matrix3::fromAxisAngleFast(rotationaxis, rotationangle), translation);
        AYAASSERT(!Math::hasNanOrInf(answer));
        return answer;
    }

    Vector3 getAxisAngle() const
    {
        return rotationaxis * rotationangle;
    }

    const Vector3& getAxis() const
    {
        return rotationaxis;
    }

    float getAngle() const
    {
        return rotationangle;
    }
};
} // namespace Aya