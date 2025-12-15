

#pragma once

#include "Utility/NormalId.hpp"

namespace Aya
{

// A utility class for holding a set of "Faces" associated with an object (top, bottom, left, right, front, back)
class Faces
{
public:
    Faces(int normalIdMask = 0);
    void clear()
    {
        normalIdMask = NORM_NONE_MASK;
    }
    void setNormalId(NormalId normalId, bool value);
    bool getNormalId(NormalId normalId) const;


    bool operator==(const Faces& other) const
    {
        return normalIdMask == other.normalIdMask;
    }
    bool operator!=(const Faces& other) const
    {
        return normalIdMask != other.normalIdMask;
    }


    int normalIdMask;
};
} // namespace Aya
