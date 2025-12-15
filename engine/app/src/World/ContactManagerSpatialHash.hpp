

#pragma once

#include "World/SpatialHashMultiRes.hpp"

namespace Aya
{
class Primitive;
class Contact;
class ContactManager;
class World;
class Assembly;

#define CONTACTMANAGER_MAXLEVELS 4
class ContactManagerSpatialHash : public SpatialHash<Primitive, Contact, ContactManager, CONTACTMANAGER_MAXLEVELS>
{
public:
    ContactManagerSpatialHash(World* world, ContactManager* contactManager);
};

} // namespace Aya
