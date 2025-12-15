

#pragma once

#include "Utility/G3DCore.hpp"
#include "Memory.hpp"

namespace Aya
{

class Body;

class Cofm : public Allocator<Cofm>
{
private:
    Body* body;
    bool dirty;
    Vector3 cofmInBody;
    float mass;
    Matrix3 moment;

    void updateIfDirty(); // true if was dirty

public:
    Cofm(Body* body);

    bool getIsDirty() const
    {
        return dirty;
    }

    void makeDirty()
    {
        dirty = true;
    }

    const Vector3& getCofmInBody()
    {
        updateIfDirty();
        return cofmInBody;
    }

    float getMass()
    {
        updateIfDirty();
        return mass;
    }

    const Matrix3& getMoment()
    {
        updateIfDirty();
        return moment;
    }
};

} // namespace Aya
