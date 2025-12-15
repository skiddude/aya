


#include "DataModel/IEquipable.hpp"

#include "DataModel/PartInstance.hpp"
#include "DataModel/JointInstance.hpp"
#include "Debug.hpp"


namespace Aya
{

IEquipable::IEquipable() {}

IEquipable::~IEquipable()
{
    AYAASSERT(!weld);
}

void IEquipable::buildWeld(PartInstance* humanoidPart, PartInstance* gadgetPart, const CoordinateFrame& humanoidCoord,
    const CoordinateFrame& gadgetCoord, const std::string& name)
{
    // Move the gadget to line up with the humanoid
    CoordinateFrame attachmentCoordWorld = humanoidPart->getCoordinateFrame() * humanoidCoord;
    CoordinateFrame gadgetCoordWorld = attachmentCoordWorld * gadgetCoord.inverse();

    //	gadgetPart->setCoordinateFrame(gadgetCoordWorld);

    // Build the weld
    if (weld.get())
    {
        AYAASSERT_FISHING(0);
        weld->setParent(NULL);
        weld.reset();
    }

    weld = Creatable<Instance>::create<Weld>();

    weld->setName(name);
    weld->setPart0(humanoidPart);
    weld->setPart1(gadgetPart);

    weld->setC0(humanoidCoord);
    weld->setC1(gadgetCoord);
    weld->setParent(humanoidPart);
}

} // namespace Aya