


#include "World/TreeStage.hpp"
#include "World/MovingStage.hpp"
#include "World/SleepStage.hpp"
#include "World/Primitive.hpp"
#include "World/RigidJoint.hpp"
#include "World/MotorJoint.hpp"
#include "World/Clump.hpp"
#include "World/Assembly.hpp"
#include "World/Mechanism.hpp"
#include "Kernel/Body.hpp"
#include "Debug.hpp"

#include <map>

namespace Aya
{

///////////////////////////////////////////////////////////////////////////////////

#pragma warning(push)
#pragma warning(disable : 4355) // 'this' : used in base member initializer list
TreeStage::TreeStage(IStage* upstream, World* world)
    : IWorldStage(upstream, new MovingStage(this, world), world)
    , maxTreeDepth(0)
{
}
#pragma warning(pop)


TreeStage::~TreeStage()
{
    AYAASSERT(dirtyMechanisms.size() == 0);
    AYAASSERT(downstreamMechanisms.size() == 0);
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

bool TreeStage::validateTree(SpanningNode* root)
{
    AYAASSERT_NOT_RELEASE();
#ifdef _DEBUG

    if (!Super::validateTree(root))
    {
        return false;
    }
    Primitive* primitive = aya_static_cast<Primitive*>(root);

    Joint* joint = aya_static_cast<Joint*>(primitive->getEdgeToParent());
    Primitive* parent = primitive->getTypedParent<Primitive>();

    Clump* parentClump = parent ? parent->getClump() : NULL;
    Clump* childClump = primitive->getClump();
    Assembly* childAssembly = primitive->getAssembly();
    Mechanism* childMechanism = primitive->getMechanism();

    AYAASSERT(childClump && childAssembly && childMechanism);

    bool isRigid = RigidJoint::isRigidJoint(joint);
    bool isMotor = Joint::isMotorJoint(joint);
    bool isSpring = Joint::isSpringJoint(joint);
    bool isGround = Joint::isGroundJoint(joint);
    bool isKinematic = (isRigid || isMotor);
    AYAASSERT(isKinematic || isSpring || isGround);
    AYAASSERT(isKinematic == Joint::isKinematicJoint(joint));

    AYAASSERT(isRigid == (parentClump == childClump));
    AYAASSERT(isRigid == (!Clump::isClumpRootPrimitive(primitive)));
    AYAASSERT(isRigid == (primitive->getTypedUpper<Clump>() == NULL));
    AYAASSERT(isKinematic == (!Assembly::isAssemblyRootPrimitive(primitive)));
    AYAASSERT(isGround == (Mechanism::isMechanismRootPrimitive(primitive)));
    AYAASSERT(isKinematic || (primitive->getBody()->getParent() == NULL));
    AYAASSERT(!isKinematic || (primitive->getBody()->getParent() == parent->getBody()));

    for (int i = 0; i < primitive->numChildren(); ++i)
    {
        Primitive* child = primitive->getTypedChild<Primitive>(i);
        AYAASSERT(child->getTypedParent<Primitive>() == primitive);
        validateTree(child);
    }
#endif
    return true;
}


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
// walk both sides up the tree to a common node, finding the lightest joint along the way
// Parent side is the opposite side from where we found the lightest
//
// existingActiveJoint will be the case where a freeJoint is added to an existing anchored primitive,
// or an anchorJoint is added to existing free primitive.  In the first case and possibly the second, there
// will be an active joint between the primitive and ground.  If active, it must be the lightest candidate



///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

bool chainToGround(Primitive* p)
{
    if (!p)
    {
        return true;
    }
    else
    {
        Primitive* root = p->getRoot<Primitive>();
        Joint* joint = aya_static_cast<Joint*>(root->getEdgeToParent());
        return (joint && Joint::isGroundJoint(joint));
    }
}

void TreeStage::onSpanningEdgeAdding(SpanningEdge* edge, SpanningNode* child)
{
#ifdef _DEBUG
    Primitive* childPrim = aya_static_cast<Primitive*>(child);
    AYAASSERT(!childPrim->getTypedUpper<Clump>());
    AYAASSERT(!childPrim->getClump());
    AYAASSERT(!childPrim->getAssembly());
    AYAASSERT(!childPrim->getMechanism());
    AYAASSERT(!chainToGround(childPrim));
#endif

    Primitive* parentPrim = aya_static_cast<Primitive*>(edge->otherNode(child));
    AYAASSERT(!parentPrim || parentPrim->getClump());
    AYAASSERT(!parentPrim || parentPrim->getAssembly());
    AYAASSERT(!parentPrim || parentPrim->getMechanism());
    AYAASSERT(!parentPrim || chainToGround(parentPrim));

    if (parentPrim)
    {
        dirtyMechanism(parentPrim->getMechanism());
    }
}


/*
        Ground:		New Mechanism
        Spring:		New Assembly
        Motor:		New Clump
        Rigid:		.....
*/

void TreeStage::onSpanningEdgeAdded(SpanningEdge* edge)
{
    Joint* joint = aya_static_cast<Joint*>(edge);
    bool isGroundJoint = Joint::isGroundJoint(joint);
    Primitive* parent = aya_static_cast<Primitive*>(edge->getParentSpanningNode());
    Primitive* child = aya_static_cast<Primitive*>(edge->getChildSpanningNode());

    AYAASSERT(chainToGround(child));
    AYAASSERT(child->getBody()->getParent() == NULL);
    AYAASSERT(!child->getTypedUpper<Clump>());
    AYAASSERT(isGroundJoint == (parent == NULL));
    AYAASSERT(isGroundJoint == (child->getTypedParent<Primitive>() == NULL));

    if (parent)
    {
        dirtyMechanism(parent->getMechanism());
    }

    if (RigidJoint::isRigidJoint(joint))
    { // RIGID JOINT - same clump
        RigidJoint* r = aya_static_cast<RigidJoint*>(joint);
        child->getBody()->setParent(parent ? parent->getBody() : NULL);
        child->getBody()->setMeInParent(r->getChildInParent(parent, child));
    }
    else
    {
#ifdef _DEBUG
        Clump* parentClump = parent ? parent->getClump() : NULL;
        AYAASSERT(isGroundJoint == (parentClump == NULL));
#endif
        Clump* childClump = new Clump();
        child->setUpper(childClump);
        if (Joint::isMotorJoint(joint))
        { // MOTOR Joint - same assembly (new clump)

#ifndef AYA_PLATFORM_IOS
            AYAASSERT(parent);
#endif

#pragma warning(push)
#pragma warning(disable : 6011)
            child->getBody()->setParent(parent->getBody());
#pragma warning(pop)
            child->getBody()->setMeInParent(joint->resetLink());
        }
        else
        {
#ifdef _DEBUG
            Assembly* parentAssembly = parent ? parent->getAssembly() : NULL;
            AYAASSERT(isGroundJoint == (parentAssembly == NULL));
#endif
            Assembly* childAssembly = new Assembly();
            childClump->setUpper(childAssembly);
            if (!isGroundJoint)
            { // Spring Joint - same mechanism (new assembly)
                AYAASSERT(Joint::isSpringJoint(joint));
            }
            else
            {
                Mechanism* childMechanism = new Mechanism(); // Ground Joint - new mechanism
                childAssembly->setUpper(childMechanism);
                AYAASSERT(childMechanism->getIndexedMeshParent() == NULL);

                dirtyMechanism(childMechanism);
            }
#ifdef _DEBUG
            AYAASSERT(childAssembly->getIndexedMeshParent() == parentAssembly);
#endif
        }
#ifdef _DEBUG
        AYAASSERT(childClump->getIndexedMeshParent() == parentClump);
#endif
    }

    sendClumpChangedMessage(child);

    AYAASSERT(child->getClump());
    AYAASSERT(child->getAssembly());
    AYAASSERT(child->getMechanism());
}

void assertNotInPipeline(Assembly* a)
{
    AYAASSERT(!a->inPipeline());
}

bool noAssembliesInPipeline(Mechanism* m)
{
    Assembly* a = m->getTypedLower<Assembly>();
    a->visitAssemblies(boost::bind(&assertNotInPipeline, _1));
    return true;
}

void TreeStage::onSpanningEdgeRemoving(SpanningEdge* edge)
{
    Primitive* child = aya_static_cast<Primitive*>(edge->getChildSpanningNode());
#ifdef _DEBUG
    Primitive* parent = aya_static_cast<Primitive*>(edge->getParentSpanningNode());
    AYA_UNUSED(parent);
    AYAASSERT_SLOW(chainToGround(parent));
    AYAASSERT_SLOW(chainToGround(child));
    AYAASSERT(child->getClump());
    AYAASSERT(child->getAssembly());
    AYAASSERT(child->getMechanism());
#endif
    dirtyMechanism(child->getMechanism());
}


void TreeStage::onSpanningEdgeRemoved(SpanningEdge* edge, SpanningNode* childNode)
{
    Joint* joint = aya_static_cast<Joint*>(edge);
    Primitive* childPrim = aya_static_cast<Primitive*>(childNode);
#ifdef _DEBUG
    Primitive* parentPrim = aya_static_cast<Primitive*>(edge->otherNode(childNode));
    AYAASSERT_SLOW(chainToGround(parentPrim));
    AYAASSERT_SLOW(!chainToGround(childPrim));
    AYAASSERT(Joint::isKinematicJoint(joint) == (childPrim->getBody()->getParent() != NULL));
#endif

    if (RigidJoint::isRigidJoint(joint))
    {
#ifdef _DEBUG
        AYAASSERT(parentPrim->getClump());
#endif
    }
    else if (Joint::isMotorJoint(joint))
    {
        destroyClump(childPrim);
    }
    else if (Joint::isSpringJoint(joint))
    {
        destroyAssembly(childPrim);
    }
    else if (Joint::isGroundJoint(joint)) // anchor or free joint
    {
        AYAASSERT(childPrim->getAssembly() == childPrim->getClump()->getTypedUpper<Assembly>());
        AYAASSERT(!childPrim->getParent());
#ifdef _DEBUG
        AYAASSERT(!parentPrim);
#endif
        destroyMechanism(childPrim);
    }
    else
    {
        AYAASSERT(0);
    }

    childPrim->getBody()->setParent(NULL);

    sendClumpChangedMessage(childPrim);

    AYAASSERT(!childPrim->getTypedUpper<Clump>());
    AYAASSERT(!childPrim->getClump());
    AYAASSERT(!childPrim->getAssembly());
    AYAASSERT(!childPrim->getMechanism());
}


void TreeStage::sendClumpChangedMessage(Primitive* childPrim)
{
    if (childPrim->getOwner())
    {
        childPrim->getOwner()->onClumpChanged();
    }
    else
    {
        // Ground Primitive
    }

    // only do the clump
    for (int i = 0; i < childPrim->numChildren(); ++i)
    {
        Primitive* childChild = childPrim->getTypedChild<Primitive>(i);
        Joint* joint = aya_static_cast<Joint*>(childChild->getEdgeToParent());
        if (RigidJoint::isRigidJoint(joint))
        {
            sendClumpChangedMessage(childChild);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

void TreeStage::removeFromPipeline(Mechanism* m)
{
    if (m->inPipeline())
    {
        if (m->downstreamOfStage(this))
        {
            aya_static_cast<MovingStage*>(getDownstreamWS())->onMechanismRemoving(m);
            int num = downstreamMechanisms.erase(m);
            AYAASSERT(num == 1);
        }
        m->removeFromPipeline(this);
    }
    AYAASSERT(noAssembliesInPipeline(m));
}

void TreeStage::dirtyMechanism(Mechanism* m)
{
    AYAASSERT(m);
    removeFromPipeline(m);
    dirtyMechanisms.insert(m);
}

void TreeStage::destroyClump(Primitive* p)
{
    Clump* c = p->getClump();
    p->setUpper(NULL);
    delete c;
}

void TreeStage::destroyAssembly(Primitive* p)
{
    Assembly* a = p->getAssembly();
    Clump* c = p->getClump();

    c->setUpper(NULL);
    p->setUpper(NULL);

    delete c;
    delete a;
}

void TreeStage::destroyMechanism(Primitive* p)
{
    Mechanism* m = p->getMechanism();
    Assembly* a = p->getAssembly();
    Clump* c = p->getClump();

    removeFromPipeline(m);
    dirtyMechanisms.erase(m);

    a->setUpper(NULL);
    c->setUpper(NULL);
    p->setUpper(NULL);

    delete a;
    delete c;
    delete m;
}


////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

void TreeStage::cleanMechanism(Mechanism* m)
{
    AYAASSERT(!m->getIndexedMeshParent());

    if (!m->inPipeline())
    {
        m->putInPipeline(this);
    }

    AYAASSERT(m->inStage(this));
    aya_static_cast<MovingStage*>(getDownstreamWS())->onMechanismAdded(m);
    bool ok = downstreamMechanisms.insert(m).second;
    AYAASSERT(ok);
}


void TreeStage::assemble()
{
    if (!isAssembled())
    {
        // 1. Assemblies
        std::set<Mechanism*>::iterator aIt;
        for (aIt = dirtyMechanisms.begin(); aIt != dirtyMechanisms.end(); ++aIt)
        {
            cleanMechanism(*aIt); // clean upstream dirtyAssemblies
        }
        dirtyMechanisms.clear();
    }

    AYAASSERT(isAssembled());
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void TreeStage::onEdgeAdded(Edge* e)
{
    AYAASSERT(e->getPrimitive(0)->inOrDownstreamOfStage(this));
    AYAASSERT(!e->getPrimitive(1) || e->getPrimitive(1)->inOrDownstreamOfStage(this));

    e->putInStage(this);

    if (Joint::isSpanningTreeJoint(e))
    {
        Joint* j = aya_static_cast<Joint*>(e);
        insertSpanningTreeEdge(j);
        if (!(RigidJoint::isRigidJoint(j) || Joint::isGroundJoint(j)))
        {
            getDownstreamWS()->onEdgeAdded(e);
        }
    }
    else
    {
        getDownstreamWS()->onEdgeAdded(e);
    }
}


void TreeStage::onEdgeRemoving(Edge* e)
{
    AYAASSERT(e->getPrimitive(0)->inOrDownstreamOfStage(this));
    AYAASSERT(!e->getPrimitive(1) || e->getPrimitive(1)->inOrDownstreamOfStage(this));

    if (Joint::isSpanningTreeJoint(e))
    {
        Joint* j = aya_static_cast<Joint*>(e);
        if (!(RigidJoint::isRigidJoint(j) || Joint::isGroundJoint(j)))
        {
            getDownstreamWS()->onEdgeRemoving(e);
        }
        if (j->inSpanningTree())
        {
            removeSpanningTreeEdge(j);
        }
    }
    else
    {
        getDownstreamWS()->onEdgeRemoving(e);
    }

    e->removeFromStage(this);
}


void TreeStage::onPrimitiveAdded(Primitive* p)
{
    AYAASSERT(p->getNumEdges() == 0);
    p->putInStage(this);
}


void TreeStage::onPrimitiveRemoving(Primitive* p)
{
    p->removeFromStage(this);
    AYAASSERT(p->getNumEdges() == 0);
}

int TreeStage::getMetric(IWorldStage::MetricType metricType)
{
    switch (metricType)
    {
    case MAX_TREE_DEPTH:
    {
        return maxTreeDepth;
    }
    default:
    {
        return IWorldStage::getMetric(metricType);
    }
    }
}

} // namespace Aya
