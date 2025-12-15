

#pragma once

#include "Utility/G3DCore.hpp"
#include "Declarations.hpp"
#include "Memory.hpp"
#include "Utility/Math.hpp"

namespace Aya
{

class Body;

class AyaBaseClass Link
{
    friend class Body;

protected:
    Body* body; // body I'm affilliated with (child)
    CoordinateFrame parentCoord;
    CoordinateFrame childCoord;
    CoordinateFrame childCoordInverse;

    CoordinateFrame childInParent;
    unsigned int stateIndex;

    virtual void computeChildInParent(CoordinateFrame& answer) const = 0;

    void dirty();

    void setBody(Body* b)
    {
        body = b;
    }

public:
    Link();

    ~Link();

    const CoordinateFrame& getChildInParent();

    Body* getBody() const
    {
        return body;
    }

    void reset(const CoordinateFrame& parentC, const CoordinateFrame& childC);
};


class RevoluteLink
    : public Link
    , public Allocator<RevoluteLink>
{
private:
    float jointAngle;

    /*override*/ void computeChildInParent(CoordinateFrame& answer) const;

public:
    RevoluteLink()
        : jointAngle(0.0f)
    {
    }

    void setJointAngle(float value)
    {
        jointAngle = value;
        dirty();
    }
};

class D6Link
    : public Link
    , public Allocator<D6Link>
{
private:
    CoordinateFrame offsetCFrame;

    /*override*/ void computeChildInParent(CoordinateFrame& answer) const;

public:
    void setJointOffsetCFrame(const CoordinateFrame& value)
    {
        offsetCFrame = value;
        AYAASSERT(!Math::hasNanOrInf(value));
        dirty();
    }
};

} // namespace Aya
