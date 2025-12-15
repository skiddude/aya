

#pragma once

#include "Kernel/ContactConnector.hpp"
#include "World/Contact.hpp"
#include "World/CellContact.hpp"

#include "BulletCollision/BroadphaseCollision/btCollisionAlgorithm.hpp"
#include "BulletCollision/CollisionDispatch/btCollisionObject.hpp"

class btPersistentManifold;

namespace Aya
{

class BulletConnector
    : public ContactConnector
    , public Allocator<BulletConnector>
{
public:
    BulletConnector(Body* b0, Body* b1, const ContactParams& contactParams, int manifoldIndex, int cacheIndex);

    int bulletManifoldIndex;
    int bulletPointCacheIndex;
};

typedef FixedArray<BulletConnector*, 4> BulletConnectorArray;

class BulletContact : public Contact
{
public:
    BulletContact(World* world, Primitive* p0, Primitive* p1);
    ~BulletContact();

    // Contact
    void deleteAllConnectors() override;
    int numConnectors() const override;
    ContactConnector* getConnector(int i) override;

    bool computeIsColliding(float overlapIgnored) override;
    bool stepContact() override;

    void invalidateContactCache() override;

private:
    World* world;

    btCollisionAlgorithm* algorithm;

    btManifoldArray manifoldArray;
    BulletConnectorArray connectors;
};

class BulletCellContact : public CellContact
{
public:
    BulletCellContact(World* world, Primitive* p0, Primitive* p1, const Vector3int32& feature, const shared_ptr<btCollisionShape>& cellShape);
    ~BulletCellContact();

    // Contact
    void deleteAllConnectors() override;
    int numConnectors() const override;
    ContactConnector* getConnector(int i) override;

    bool computeIsColliding(float overlapIgnored) override;
    bool stepContact() override;

    void invalidateContactCache() override;
    void onPrimitiveContactParametersChanged() override;

private:
    World* world;

    btCollisionAlgorithm* algorithm;

    btManifoldArray manifoldArray;
    BulletConnectorArray connectors;

    btCollisionObject cellCollisionObject; // collision object for the cell involved in contact
    shared_ptr<btCollisionShape> cellShape;

    void updateContactParemeters(btCollisionObject* cellObj);
};

} // namespace Aya
