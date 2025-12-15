

#include "World/ContactManagerSpatialHash.hpp"
#include "World/ContactManager.hpp"
#include "World/Contact.hpp"
#include "World/Assembly.hpp"


// instantiate for contactmanager.
template class Aya::SpatialHash<Aya::Primitive, Aya::Contact, Aya::ContactManager, CONTACTMANAGER_MAXLEVELS>;

#define CONTACT_MANAGER_MAX_CELLS_PER_PRIMITIVE 100

namespace Aya
{

ContactManagerSpatialHash::ContactManagerSpatialHash(World* world, ContactManager* contactManager)
    : SpatialHash<Primitive, Contact, ContactManager, CONTACTMANAGER_MAXLEVELS>(
          world, contactManager, CONTACT_MANAGER_MAX_CELLS_PER_PRIMITIVE) // max cells per primitive is 100 for contact manager
{
}

} // namespace Aya
