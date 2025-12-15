


#include "World/JointStage.hpp"
#include "World/GroundStage.hpp"
#include "World/World.hpp"
#include "World/Primitive.hpp"
#include "World/Joint.hpp"
#include "World/Assembly.hpp"
#include "Debug.hpp"



namespace Aya
{

#pragma warning(push)
#pragma warning(disable : 4355) // 'this' : used in base member initializer list
JointStage::JointStage(IStage* upstream, World* world)
    : IWorldStage(upstream, new GroundStage(this, world), world)
{
}
#pragma warning(pop)

JointStage::~JointStage()
{
    AYAASSERT(jointMap.empty());
    AYAASSERT(incompleteJoints.empty());
    AYAASSERT(primitivesHere.empty());
}


GroundStage* JointStage::getGroundStage()
{
    return aya_static_cast<GroundStage*>(getDownstream());
}

void JointStage::moveEdgeToDownstream(Edge* e)
{
    AYAASSERT(edgeHasPrimitivesHere(e));
    getGroundStage()->onEdgeAdded(e);
}


void JointStage::removeEdgeFromDownstream(Edge* e)
{
    AYAASSERT(edgeHasPrimitivesHere(e));
    getGroundStage()->onEdgeRemoving(e);
}

void JointStage::moveJointToDownstream(Joint* j)
{
    AYAASSERT_SLOW(incompleteJoints.find(j) == incompleteJoints.end());
    AYAASSERT_SLOW(!jointMap.pairInMap(j->getPrimitive(0), j));
    AYAASSERT_SLOW(!jointMap.pairInMap(j->getPrimitive(1), j));
    moveEdgeToDownstream(j);
}


void JointStage::removeJointFromDownstream(Joint* j)
{
    AYAASSERT_SLOW(incompleteJoints.find(j) == incompleteJoints.end());
    AYAASSERT_SLOW(!jointMap.pairInMap(j->getPrimitive(0), j));
    AYAASSERT_SLOW(!jointMap.pairInMap(j->getPrimitive(1), j));
    removeEdgeFromDownstream(j);
}

bool JointStage::edgeHasPrimitiveHere(Edge* e, Primitive* p)
{
    AYAASSERT(e->links(p));
    return (p && (primitivesHere.find(p) != primitivesHere.end()));
}


bool JointStage::edgeHasPrimitivesHere(Edge* e)
{
    return (edgeHasPrimitiveHere(e, e->getPrimitive(0)) && edgeHasPrimitiveHere(e, e->getPrimitive(1)));
}

void JointStage::visitAddedPrimitive(Primitive* p, Joint* j, std::vector<Joint*>& jointsToPush)
{
    AYAASSERT(edgeHasPrimitiveHere(j, p));

    if (edgeHasPrimitiveHere(j, j->otherPrimitive(p)))
    {
        jointsToPush.push_back(j);
    }
}


void JointStage::onPrimitiveAdded(Primitive* p)
{
    WriteValidator validator(concurrencyValidator);

    bool ok = primitivesHere.insert(p).second;
    AYAASSERT(ok);


    p->putInStage(this);
    getGroundStage()->onPrimitiveAdded(p); // this should now make some joints ok - their primitives are in the state

    std::vector<Joint*> jointsToPush;

    jointMap.visitEachLeft(p, boost::bind(&JointStage::visitAddedPrimitive, this, _1, _2, boost::ref(jointsToPush)));

    for (size_t i = 0; i < jointsToPush.size(); ++i)
    {
        Joint* j = jointsToPush[i];
        removeJointFromHere(j);
        moveJointToDownstream(j);
    }
}

void JointStage::onPrimitiveRemoving(Primitive* p)
{
    WriteValidator validator(concurrencyValidator);

    AYAASSERT(p->getNumContacts() == 0);

    std::vector<Joint*> jointsToPop;

    Joint* j = p->getFirstJoint(); // all Primitive Joints are by definition downstream
    while (j)
    {
        if (!AnchorJoint::isAnchorJoint(j) && !FreeJoint::isFreeJoint(j))
        {
            if (j->downstreamOfStage(this))
            {
                AYAASSERT(edgeHasPrimitivesHere(j));
                jointsToPop.push_back(j);
            }
        }
        j = p->getNextJoint(j);
    }


    for (size_t i = 0; i < jointsToPop.size(); ++i)
    {
        Joint* pop = jointsToPop[i];

        removeJointFromDownstream(pop);

        putJointHere(pop);
    }

    AYAASSERT(p->getNumJoints() == 1); // should have one free or anchor joint
    getGroundStage()->onPrimitiveRemoving(p);
    p->removeFromStage(this);

    int num = primitivesHere.erase(p);
    AYAASSERT(num == 1);
}


void JointStage::putJointHere(Joint* j)
{
    bool ok = incompleteJoints.insert(j).second;
    AYAASSERT(ok);
    jointMap.insertPair(j->getPrimitive(0), j);
    jointMap.insertPair(j->getPrimitive(1), j);
}

void JointStage::removeJointFromHere(Joint* j)
{
    int num = incompleteJoints.erase(j);
    AYAASSERT(num == 1);
    jointMap.removePair(j->getPrimitive(0), j);
    jointMap.removePair(j->getPrimitive(1), j);
}

void JointStage::onEdgeAdded(Edge* e)
{
    WriteValidator validator(concurrencyValidator);

    AYAASSERT(e->getPrimitive(0) && e->getPrimitive(1) && (e->getPrimitive(0) != e->getPrimitive(1)));

    e->putInStage(this);

    if (Joint::isJoint(e))
    {
        if (edgeHasPrimitivesHere(e))
        {
            moveEdgeToDownstream(e);
        }
        else
        {
            Joint* j = aya_static_cast<Joint*>(e);
            putJointHere(j);
        }
    }
    else
    {
        AYAASSERT(edgeHasPrimitivesHere(e)); // contacts - both primitives should be here
        moveEdgeToDownstream(e);
    }
}


void JointStage::onEdgeRemoving(Edge* e)
{
    WriteValidator validator(concurrencyValidator);

    AYAASSERT(e->getPrimitive(0) && e->getPrimitive(1) && (e->getPrimitive(0) != e->getPrimitive(1)));

    if (e->downstreamOfStage(this))
    {
        AYAASSERT(edgeHasPrimitivesHere(e));
        removeEdgeFromDownstream(e);
    }
    else
    {
        AYAASSERT(Joint::isJoint(e));
        Joint* j = aya_static_cast<Joint*>(e);
        removeJointFromHere(j);
    }

    e->removeFromStage(this);
}


} // namespace Aya