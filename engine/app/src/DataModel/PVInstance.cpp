


#include "DataModel/PVInstance.hpp"
#include "DataModel/UserController.hpp"
#include "DataModel/Workspace.hpp"
#include "Utility/Math.hpp"
#include "Utility/Action.hpp"
#include "Debug.hpp"
#include "DrawAdorn.hpp"
#include "Base/Part.hpp"
#include "Base/Adorn.hpp"



namespace Aya
{

const char* const sPVInstance = "PVInstance";

///////////////////////////
// legacy stuff - old offset


static const Reflection::PropDescriptor<PVInstance, CoordinateFrame> desc_CoordFrame(
    "CoordinateFrame", category_Data, NULL, &PVInstance::setPVGridOffsetLegacy, Reflection::PropertyDescriptor::UI);
REFLECTION_END();

PVInstance::PVInstance(const char* name)
    : Reflection::Described<PVInstance, sPVInstance, Instance>(name)
{
}

void PVInstance::moveToPointNoUnjoinNoJoin(Vector3 point)
{
    if (Workspace* workspace = Workspace::findWorkspace(this))
    {
        workspace->moveToPoint(this, point, DRAG::NO_UNJOIN_NO_JOIN);
    }
}


void PVInstance::moveToPointAndJoin(Vector3 point)
{
    if (Workspace* workspace = Workspace::findWorkspace(this))
    {
        workspace->moveToPoint(this, point, DRAG::UNJOIN_JOIN);
    }
}

void PVInstance::moveToPointNoJoin(Vector3 point)
{
    if (Workspace* workspace = Workspace::findWorkspace(this))
    {
        workspace->moveToPoint(this, point, DRAG::UNJOIN_NO_JOIN);
    }
}

void PVInstance::readProperty(const XmlElement* propertyElement, IReferenceBinder& binder)
{

    // If the element is an Feature and the Feature does not exist, create it
    if (propertyElement->getTag() == "Feature")
    {
        const Aya::Name* name = NULL;
        bool foundNameAttribute = propertyElement->findAttributeValue(name_name, name);
        AYAASSERT(foundNameAttribute);
        if (*name == "Part")
        {
            // Read legacy PartInstance data
            this->readProperties(propertyElement, binder);
            return;
        }
        else if (*name == "Item")
        {
            // Read legacy PVAttribute data
            this->readProperties(propertyElement, binder);
            return;
        }
    }

    Super::readProperty(propertyElement, binder);
}


PVInstance::~PVInstance() {}


PVInstance* PVInstance::getTopLevelPVParent()
{
    if (isTopLevelPVInstance())
    { // no parent, or parent==root
        return this;
    }
    else
    {
        return getTypedParent<PVInstance>()->getTopLevelPVParent();
    }
}

const PVInstance* PVInstance::getTopLevelPVParent() const
{
    if (isTopLevelPVInstance())
    { // no parent, or parent==root
        return this;
    }
    else
    {
        return getTypedParent<PVInstance>()->getTopLevelPVParent();
    }
}

void PVInstance::renderCoordinateFrame(Adorn* adorn)
{
    adorn->setObjectToWorldMatrix(getLocation());
    adorn->axes(G3D::Color3::red(), G3D::Color3::green(), G3D::Color3::blue(), 10.0);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////


// UI versions
void PVInstance::setPVGridOffsetLegacy(const CoordinateFrame& _offset)
{
    throw Aya::runtime_error("CoordinateFrame is not a valid member of %s", getDescriptor().name.c_str());
}




} // namespace Aya