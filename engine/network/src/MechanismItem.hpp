#pragma once

#include "DataModel/PartInstance.hpp"

#include "Utility/PV.hpp"

#include "Memory.hpp"

#include "Debug.hpp"

#include "Array.hpp"
#include "CompactCFrame.hpp"
#include "RakNet/RakNetTime.hpp"

namespace Aya
{

class Primitive;

class AssemblyItem : public boost::noncopyable
{
public:
    shared_ptr<PartInstance> rootPart;
    Aya::PV pv;
    G3D::Array<CompactCFrame> motorAngles;

    void reset()
    {
        rootPart.reset();
        motorAngles.fastClear();
    }

    AssemblyItem()
    {
        reset();
    }
};


class MechanismItem : boost::noncopyable
{
private:
    // TODO: Replace with intrusive list of AssemblyItem
    G3D::Array<AssemblyItem*> buffer;

    int currentElements;

public:
    RakNet::Time networkTime;
    unsigned char networkHumanoidState;
    bool hasVelocity;

    void reset(int numElements = 0);

    MechanismItem()
    {
        reset();
    }

    ~MechanismItem();

    AssemblyItem& appendAssembly();

    int numAssemblies() const
    {
        return currentElements;
    }

    AssemblyItem& getAssemblyItem(int i) const
    {
        AYAASSERT(i < currentElements);
        return *buffer[i];
    }

    static bool consistent(const MechanismItem* before, const MechanismItem* after);

    static void lerp(const MechanismItem* before, const MechanismItem* after, MechanismItem* out, float lerpAlpha);
};
} // namespace Aya
