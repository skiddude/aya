

#include "MechanismItem.hpp"
#include "Utility/PV.hpp"

#include "Utility/Math.hpp"

#include "Debug.hpp"


using namespace Aya;

MechanismItem::~MechanismItem()
{
    for (int i = 0; i < buffer.size(); ++i)
        delete buffer[i];
}

void MechanismItem::reset(int numElements)
{
    networkHumanoidState = 0;
    hasVelocity = false;
    networkTime = 0;
    currentElements = numElements;
    while (buffer.size() < numElements)
        buffer.append(new AssemblyItem());
}

AssemblyItem& MechanismItem::appendAssembly()
{
    AYAASSERT(buffer.size() >= currentElements);

    if (buffer.size() == currentElements)
        buffer.append(new AssemblyItem());
    else
    {
        AYAASSERT(buffer[currentElements] != NULL);
        buffer[currentElements]->reset();
    }

    AssemblyItem& answer = *buffer[currentElements];

    currentElements++;

    return answer;
}

/*
        For now - no lerp on mechanisms (>1 assembly)
*/

bool MechanismItem::consistent(const MechanismItem* before, const MechanismItem* after)
{
    AYAASSERT(before && after);
    AYAASSERT(before->networkTime < after->networkTime);

    return ((before->hasVelocity == after->hasVelocity) && (before->numAssemblies() == 1) && (after->numAssemblies() == 1) &&
            (before->getAssemblyItem(0).motorAngles.size() == after->getAssemblyItem(0).motorAngles.size()));
}


void MechanismItem::lerp(const MechanismItem* before, const MechanismItem* after, MechanismItem* out, float lerpAlpha)
{
    AYAASSERT(consistent(before, after));
    AYAASSERT(before->numAssemblies() == 1);

    out->reset(before->numAssemblies());

    out->networkTime = 0; // shouldn't be used
    out->networkHumanoidState = after->networkHumanoidState;
    out->hasVelocity = after->hasVelocity;

    AssemblyItem& beforeA = before->getAssemblyItem(0);
    AssemblyItem& afterA = after->getAssemblyItem(0);
    AssemblyItem& outA = out->getAssemblyItem(0);

    outA.rootPart = afterA.rootPart;

    outA.pv = beforeA.pv.lerp(afterA.pv, lerpAlpha);

    outA.motorAngles.resize(beforeA.motorAngles.size()); // should prevent reallocs
    for (int i = 0; i < beforeA.motorAngles.size(); ++i)
    {
        outA.motorAngles[i] = CompactCFrame(beforeA.motorAngles[i].translation.lerp(afterA.motorAngles[i].translation, lerpAlpha),
            beforeA.motorAngles[i].getAxisAngle().lerp(afterA.motorAngles[i].getAxisAngle(), lerpAlpha));
    }
}
